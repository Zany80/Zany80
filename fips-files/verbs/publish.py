import subprocess

from mod import log

def run(fips_dir, proj_dir, args) :
    # It would be nice to actually do the work in here, but my knowledge of Bash
    # far outpaces my knowledge of Python right now. TODO: rewrite the bash
    # script in here.
    subprocess.call('{}/tools/publish.sh'.format(proj_dir), cwd = proj_dir, shell = False)

#-------------------------------------------------------------------------------
def help() :
    log.info(log.YELLOW +
             'fips publish\n' +
             log.DEF +
             '    publish Emscripten build to Zany80.github.io')

