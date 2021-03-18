from distutils.core import setup, Extension

# dammit beavis library names are tcl86 on freebsd on tcl8.6 on mac

module1 = Extension('tclpy',
                    define_macros = [('PACKAGE_VERSION', '"0.5.0"')],
                    libraries = ['tcl86'],
                    include_dirs = ['/usr/local/include', '/usr/local/include/tcl8.6'],
                    library_dirs = ['/usr/local/lib'],
                    sources = ['generic/tclpy.c'])

setup (name = 'tclpy',
       version = '0.5.0',
       description = 'python-tcl integration',
       ext_modules = [module1])
