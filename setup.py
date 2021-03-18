from distutils.core import setup, Extension

# dammit beavis library names are tcl86 on freebsd on tcl8.6 on mac

module1 = Extension('tohil',
                    define_macros = [('PACKAGE_VERSION', '"1.0.0"')],
                    libraries = ['tcl86'],
                    include_dirs = ['/usr/local/include', '/usr/local/include/tcl8.6'],
                    library_dirs = ['/usr/local/lib'],
                    sources = ['generic/tohil.c'])

setup (name = 'tohil',
       version = '1.0.0',
       description = 'python-tcl integration',
       ext_modules = [module1])
