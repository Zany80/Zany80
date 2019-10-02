#include <unistd.h>

#include "SIMPLE/3rd-party/stretchy_buffer.h"
#include "SIMPLE/3rd-party/cfgpath.h"
#include "SIMPLE/API.h"
#include "SIMPLE/XML.h"
#include "SIMPLE/Plugin.h"
#include "SIMPLE/repository.h"

#include <git2.h>

#include <threads.h>
#include <stdatomic.h>

static thrd_t **threads = NULL;
static atomic_int **statuses = NULL;
static char ** local_paths = NULL;
static char ** remote_paths = NULL;
//~ static atomic_bool shutting_down = false;

enum {
	RESULT_INCOMPLETE, /// thread hasn't finished yet!
	RESULT_THRD_FAILURE, /// error spawning thread
	RESULT_SUCCESS, /// repo cloned successfully
	RESULT_ERROR, /// error cloning repo
	RESULT_ALREADY_EXISTS, /// repo already cloned!
};

int clone_thread(void *_index) {
	int index = *((int*)_index);
	free(_index);
	git_repository *repo = NULL;
	int result;
	char *local_path = local_paths[index];
	char *path = remote_paths[index];
	simple_log(SL_INFO, "Cloning '%s' to '%s'...\n", path, local_path);
	// TODO for 0.2: add progress callback and cancel when shutting down.
	// For now, leaving the thread running in the background isn't a big deal
	int code = git_clone(&repo, path, local_path, NULL);
	if (code >= 0) {
		git_repository_free(repo);
		result = RESULT_SUCCESS;
		simple_log(SL_DEBUG, "Cloned successfully!\n");
	}
	else {
		const git_error *e = git_error_last();
		simple_log(SL_ERROR, "Error cloning repo %s: %s!\n", local_path, e ? e->message : NULL);
		if (code == GIT_EEXISTS) {
			result = RESULT_ALREADY_EXISTS;
		}
		else {
			result = RESULT_ERROR;
		}
	}
	atomic_store(statuses[index], result);
	return 0;
}

// must be run from main thread!
void launch_repo_thread(char *const path) {
	// TODO: avoid collisions (e.g. github.com/x/y and github.com/a/y or git.sr.ht/~x/y)
	char *last_part = strrchr(path, '/');
	if (last_part != NULL) {
		last_part++;
		simple_log(SL_DEBUG, "Local directory name: %s\n", last_part);
		char *local_path = malloc(MAX_PATH);
		get_user_data_folder(local_path, MAX_PATH, "SIMPLE");
		simple_log(SL_DEBUG, "Data folder: %s\n", local_path);
		if (access(local_path, F_OK) != 0) {
			mkdir(local_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		}
		strcat(local_path, last_part);
		simple_log(SL_DEBUG, "Repository folder: %s\n", local_path);
		int *index = malloc(sizeof(int));
		*index = sb_count(threads);
		{
			atomic_int *a = malloc(sizeof(atomic_int));
			atomic_store(a, RESULT_INCOMPLETE);
			sb_push(statuses, a);
		}
		sb_push(local_paths, local_path);
		sb_push(remote_paths, path);
		thrd_t *thread = malloc(sizeof(thrd_t));
		if (thrd_create(thread, clone_thread, index) != thrd_success) {
			atomic_store(statuses[*index], RESULT_THRD_FAILURE);
			free(index);
		}
		// If failure, still push the thread; it'll be cleaned up later
		sb_push(threads, thread);
	}
	else {
		simple_log(SL_ERROR, "Invalid url: %s\n", path);
		simple_report_error("Invalid URL!");
		free(path);
	}
}

#ifdef ORYOL_WINDOWS
const char *const PLAT = "Windows";
#elif defined(ORYOL_LINUX)
const char *const PLAT = "Linux";
#else
#error Unsupported platform!
#endif

static window_t *update_window = NULL;
static widget_t **update_groups = NULL;
static char **update_paths = NULL;
static int discard = -1;

static char *git_error_message() {
	const git_error *e = git_error_last();
	if (e) {
		return e->message;
	}
	return NULL;
}

void repository_update(const char *const path) {
	// git fetch, then re-register repo
	git_repository *repo = NULL;
	simple_log(SL_DEBUG, "Updating repo '%s'\n", path);
	if (git_repository_open(&repo, path) == 0) {
		git_remote *remote = NULL;
		if (git_remote_lookup(&remote, repo, "origin") == 0) {
			git_fetch_options opts = GIT_FETCH_OPTIONS_INIT;
			if (git_remote_fetch(remote, NULL, &opts, NULL) >= 0) {
				simple_log(SL_DEBUG, "Remote fetched successfully, checking out master branch...\n");
				git_object *treeish = NULL;
				if (git_revparse_single(&treeish, repo, "origin/master") == 0) {
					git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
					opts.checkout_strategy = GIT_CHECKOUT_FORCE;
					if (git_checkout_tree(repo, treeish, &opts) == 0 && 
							git_repository_set_head(repo, "refs/heads/master") == 0) {
						simple_log(SL_DEBUG, "Checked out origin/master of '%s' w/o issues.\n", path);
						repository_register(path, true);
					}
					else {
						simple_report_error("Error checking out new files!");
					}
					git_object_free(treeish);
				}
				else {
					simple_report_error("Error locating master branch");
				}
			}
			else {
				simple_report_error("Error fetching data! Are you connected to the internet?");
				simple_log(SL_ERROR, "Error fetching from remote: '%s'\n", git_error_message());
			}
		}
		else {
			simple_report_error("Error retrieving source information!");
			simple_log(SL_ERROR, "Error retrieving info on remote 'origin'!\n");
		}
	}
	else {
		simple_log(SL_ERROR, "Error opening repository '%s'!\n", path);
		simple_report_error("Error opening repository for updating!");
	}
}

static void do_update(int index) {
	simple_log(SL_DEBUG, "Updating repository '%s'...\n", update_paths[index]);
	repository_update(update_paths[index]);
	discard = -1;
}

static void dont_update(int index) {
	window_remove(update_window, update_groups[index]);
	sb_remove_i((void***)&update_groups, index);
	sb_remove_i((void***)&update_paths, index);
	if (sb_count(update_groups) == 0) {
		window_unregister(update_window);
	}
	discard = -1;
}

static void recommend_update(const char *const path, const char *const message) {
	if (!update_window) {
		update_window = window_create("Automatic Updater");
		window_auto_size(update_window, true);
	}
	for (int i = 0; i < sb_count(update_paths); i++) {
		if (!strcmp(update_paths[i], path)) {
			return;
		}
	}
	widget_t *update_group = group_create();
	window_append(update_window, update_group);
	sb_push(update_groups, update_group);
	sb_push(update_paths, strdup(path));
	group_add(update_group, label_set_wrapped(label_create(message), true));
	group_add(update_group, label_set_wrapped(label_create("\nDo you wish to update? This will update all plugins in the following repository: "), true));
	group_add(update_group, label_set_wrapped(label_create(path), false));
	char buf[32];
	sprintf(buf, "Yes##%d", sb_count(update_paths));
	group_add(update_group, radio_create(buf, &discard, sb_count(update_groups) - 1, do_update));
	sprintf(buf, "No##%d", sb_count(update_paths));
	group_add(update_group, radio_create(buf, &discard, sb_count(update_groups) - 1, dont_update));
	if (sb_count(update_groups) == 1) {
		window_register(update_window);
	}
}

void updater_cleanup() {
	if (update_window) {
		if (sb_count(update_groups) > 0) {
			window_unregister(update_window);
		}
		window_destroy(update_window);
		for (int i = 0; i < sb_count(update_groups); i++) {
			group_clear(update_groups[i], true);
			widget_destroy(update_groups[i]);
			free(update_paths[i]);
		}
		sb_free(update_groups);
		sb_free(update_paths);
	}
}

void repository_register(const char *const path, bool update) {
	simple_log(SL_DEBUG, "Analyzing repository '%s'...\n", path);
	char *manifest_path = malloc(strlen(path) + 14);
	strcat(strcpy(manifest_path, path), "/manifest.xml");
	if (access(manifest_path, F_OK) == 0) {
		simple_log(SL_DEBUG, "\tAnalyzing manifest...\n");
		xml_document_t manifest = h_document_read(manifest_path);
		if (manifest) {
			xml_node_t r = document_get_root(manifest);
			bool any_installed = false;
			node_for_each(r, "Plugin", plugin, {
				simple_log(SL_INFO, "\tProcessing plugin '%s'...\n", node_get_attribute(plugin, "name"));
				bool outdated = false;
				node_for_each(plugin, "Version", v, {
					int ABI = node_get_attribute_i(v, "ABI");
					const char *const vID = node_get_attribute(v, "id");
					if (ABI == 0) {
						simple_log(SL_WARN, "\t\tInvalid version '%s': no ABI (or an invalid one) specified!\n", node_get_attribute(v, "id"));
					}
					if (!vID) {
						simple_log(SL_WARN, "\t\tInvalid version: no version ID specified!\n");
					}
					if (vID != NULL && ABI != 0) {
						bool valid_abi = true;
						if (ABI != SIMPLE_ABI) {
							// at the moment, there's only one ABI, so if it
							// doesn't match, reject it
							// hypothetically, there might be a use for other
							// ABIs later - e.g. if stuff is *added* in 3, and
							// that's altered in 4, then both 2 and 4 are valid,
							// as nothing in the ABI for 2 is invalid, while 3
							// is no longer valid, as it was altered in four
							simple_log(SL_WARN, "\t\tVersion '%s' uses the incompatible ABI version %d!\n", vID, ABI);
							valid_abi = false;
						}
						xml_node_t platform = NULL;
						node_for_each(v, "Platform", p, {
							if (!strcmp(node_get_attribute(p, "id"), PLAT)) {
								platform = p;
								break;
							}
						});
						if (platform != NULL) {
							if (valid_abi) {
								simple_log(SL_INFO, "\t\tPlugin version '%s' found with %s support: '%s'!\n", node_get_attribute(v, "id"), PLAT, node_get_value(platform));
								char *full_path = malloc(strlen(path) + strlen(node_get_value(platform)) + 2);
								strcat(strcat(strcpy(full_path, path), "/"), node_get_value(platform));
								bool process = true;
								if (update) {
									plugin_config_t config = config_load();
									for (size_t i = 0; i < config->known_plugins; i++) {
										if (!strcmp(config->plugins[i].path, full_path)) {
											if (!strcmp(node_get_attribute(v, "id"), config->plugins[i].version)) {
												process = false;
												simple_log(SL_DEBUG, "\t\t\tAlready installed, skipping...\n");
												break;
											}
										}
									}
								}
								if (process) {
									any_installed = true;
									register_plugin(full_path, path, node_get_attribute(v, "id"));
								}
								free(full_path);
							}
							else {
								outdated = true;
							}
						}
					}
				});
				if (outdated) {
					simple_log(SL_WARN, "\t\tOutdated plugin!");
					recommend_update(path, 	"The repository contains outdated plugins, which can no longer be loaded by SIMPLE.");
				}
			});
			if (!any_installed && update) {
				simple_report_error("Already up to date!");
			}
		}
		else {
			simple_report_error("Error parsing manifest!");
		}
		document_destroy(manifest);
	}
	else {
		simple_log(SL_ERROR, "\tManifest not found!");
		simple_report_error("Invalid repository: manifest.xml not found!");
	}
	free(manifest_path);
}

// must be run from main thread!
void process_threads() {
	static int wait = 0;
	for (int i = 0; i < sb_count(threads); i++) {
		int status = atomic_load(statuses[i]);
		switch (status) {
			case RESULT_THRD_FAILURE:
				simple_report_error("Error spawning download thread!");
				break;
			case RESULT_INCOMPLETE:
				if (wait-- < 0) {
					simple_log(SL_DEBUG, "Still waiting on download thread for '%s'...\n", local_paths[i]);
					wait = 60;
				}
				break;
			case RESULT_SUCCESS:
				repository_register(local_paths[i], false);
				break;
			case RESULT_ALREADY_EXISTS:
				simple_report_error("Already downloaded!");
				break;
			case RESULT_ERROR:
				simple_report_error("Error cloning repository!");
				break;
		}
		if (*statuses[i] != RESULT_INCOMPLETE) {
			free(local_paths[i]);
			free(remote_paths[i]);
			free(statuses[i]);
			free(threads[i]);
			sb_remove_i((void***)&local_paths, i);
			sb_remove_i((void***)&threads, i);
			sb_remove_i((void***)&statuses, i);
			sb_remove_i((void***)&remote_paths, i);
		}
	}
}

void repoclone_cleanup() {
	// TODO: implement this (see TODO for 0.2 above)
}