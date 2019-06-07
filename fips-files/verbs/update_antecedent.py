import subprocess

from mod import log, util, config, project, settings
def run(fips_dir, proj_dir, args) :
    if not util.is_valid_project_dir(proj_dir) :
        log.error('must be run in a project directory')
    cfg_name = None
    if len(args) > 0 :
        cfg_name = args[0]
    if not cfg_name :
        cfg_name = settings.get(proj_dir, 'config')
    deploy_dir = util.get_deploy_dir(fips_dir, util.get_project_name_from_dir(proj_dir), cfg_name)
    subprocess.call('curl https://raw.githubusercontent.com/limnarch/vm/master/bin/boot.bin -o {}/data/antecedent.rom'.format(deploy_dir), cwd = proj_dir, shell = False)
#-------------------------------------------------------------------------------
def help() :
    log.info(log.YELLOW +
            "fips update_antecedent\n" + log.DEF + 
            "   updates the antecedent firmware used by the local build")
