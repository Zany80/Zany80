"""build fips project
build
build [config]
"""

from mod import log, util, project, settings
from glob import glob
from os import makedirs
from shutil import copy
from os.path import basename

#-------------------------------------------------------------------------------
def run(fips_dir, proj_dir, args) :
    # Call the normal build verb
    """build fips project"""
    if not util.is_valid_project_dir(proj_dir) :
        log.error('must be run in a project directory')
    cfg_name = None
    if len(args) > 0 :
        cfg_name = args[0]
    if not cfg_name :
        cfg_name = settings.get(proj_dir, 'config')
    project.build(fips_dir, proj_dir, cfg_name)
    deploy_dir = util.get_deploy_dir(fips_dir, util.get_project_name_from_dir(proj_dir), cfg_name)
    makedirs('{}/plugins'.format(deploy_dir), exist_ok = True)
    plugin_extension = None
    if 'emsc' in cfg_name:
        plugin_extension = 'js'
    elif 'wasm' in cfg_name:
        plugin_extension = 'wasm'
    elif 'win' in cfg_name:
        plugin_extension = 'dll'
    else:
        plugin_extension = 'so'
    for i in glob('{}/*.{}'.format(deploy_dir, plugin_extension)):
        if not 'zany80.{}'.format(plugin_extension) in i:
            name = basename(i)
            log.info(log.YELLOW + 'Copying {} into {}/plugins/'.format(i, deploy_dir) + log.DEF)
            copy(i, '{}/plugins/{}'.format(deploy_dir,name))
    makedirs('{}/data'.format(deploy_dir), exist_ok = True)
    makedirs('{}/lib'.format(deploy_dir), exist_ok = True)
    log.info(log.YELLOW + 'Copying {}/misc/stdlib.o into {}/lib/'.format(proj_dir, deploy_dir) + log.DEF)
    copy('{}/misc/stdlib.o'.format(proj_dir), '{}/lib/stdlib.o'.format(deploy_dir))

#-------------------------------------------------------------------------------
def help() :
    """print build help"""
    log.info(log.YELLOW + 
            "fips build\n" 
            "fips build [config]\n" + log.DEF + 
            "    perform a build for current or named config\n"
            "    This overrides the standards \"build\" verb, fixing the "
            "locations of the plugins so that Zany80 can be tested more easily.")
