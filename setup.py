from distutils.core import setup, Extension

module1 = Extension('tclpy',
                    define_macros = [('PACKAGE_VERSION', '"0.5.0"')],
                    libraries = ['tcl8.6'],
                    library_dirs = ['/usr/local/lib'],
                    sources = ['generic/tclpy.c'])

setup (name = 'tclpy',
       version = '0.5.0',
       description = 'python-tcl integration',
       ext_modules = [module1])
