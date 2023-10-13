# Development environment
Best way how to build WebastoTraceability on MS Windows platform is to install [Msys2](https://www.msys2.org/) environment. 
After that there is several dependencies:
* gcc
* make
* pkg-config
* Gtk3
* SQLite3
* OpenSSL

For installation in msys2 platfor use following commands:
```
# pacman -Syu
# pacman -S mingw-w64-x86_64-toolchain base-devel pkg-config make
# pacman -S mingw-w64-x86_64-gtk3
# pacman -S openssl openssl-devel
# pacman -S mingw-w64-x86_64-sqlite3
```

After instalation of dependencies, is nececery to set Windows Environment Variable 'PATH' to:
```
C:\msys64\usr\bin;C:\msys64\mingw64\bin.
```

This is nececery for MS system to see Msys2 environment tools. Next step is to set MS envirionemnt Variable 'PKG_CONFIG_PATH':
```
C:\msys64\mingw64\lib\pkgconfig;C:\msys64\usr\lib\pkgconfig
```

This is nececery for pkg_config where to search .pc files with build configurations for given library.
After this steps, to prepare project to build run commands:

```
# make clean
# make dep
```

For building the project run command:

```
# make
```

and run it by running command:

```
# make exec
```

It is also possible to open project in IDE for example [Eclipse CDT](https://projects.eclipse.org/projects/tools.cdt)

