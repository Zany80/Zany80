#include <unistd.h>

#include "SIMPLE/3rd-party/stretchy_buffer.h"
#include "SIMPLE/3rd-party/cfgpath.h"
#include "SIMPLE/API.h"
#include "SIMPLE/XML.h"
#include "SIMPLE/Plugin.h"

#include <git2.h>

#include <threads.h>
#include <stdatomic.h>

static thrd_t **threads = NULL;
static atomic_int **statuses = NULL;
static char ** local_paths = NULL;
static char ** remote_paths = NULL;

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
	git_libgit2_init();
	git_repository *repo = NULL;
	int result;
	char *local_path = local_paths[index];
	char *path = remote_paths[index];
	simple_log(SL_INFO, "Cloning '%s' to '%s'...\n", path, local_path);
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
	git_libgit2_shutdown();
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

void register_plugin_repository(const char *const path) {
	simple_log(SL_DEBUG, "Analyzing repository '%s'...\n", path);
	char *manifest_path = malloc(strlen(path) + 14);
	strcat(strcpy(manifest_path, path), "/manifest.xml");
	if (access(manifest_path, F_OK) == 0) {
		simple_log(SL_DEBUG, "\tAnalyzing manifest...\n");
		xml_document_t manifest = h_document_read(manifest_path);
		if (manifest) {
			xml_node_t r = document_get_root(manifest);
			node_for_each(r, "Plugin", plugin, {
				simple_log(SL_INFO, "\tProcessing plugin '%s'...\n", node_get_attribute(plugin, "name"));
				node_for_each(plugin, "Version", v, {
					xml_node_t platform = NULL;
					node_for_each(v, "Platform", p, {
						if (!strcmp(node_get_attribute(p, "id"), PLAT)) {
							platform = p;
							break;
						}
					});
					if (platform != NULL) {
						simple_log(SL_INFO, "\t\tPlugin version '%s' found with %s support: '%s'!\n", node_get_attribute(v, "id"), PLAT, node_get_value(platform));
						char *full_path = malloc(strlen(path) + strlen(node_get_value(platform)) + 2);
						strcat(strcat(strcpy(full_path, path), "/"), node_get_value(platform));
						register_plugin(full_path, path, node_get_attribute(v, "id"));
						free(full_path);
					}
				});
			});
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
				register_plugin_repository(local_paths[i]);
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