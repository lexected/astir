# Setting up

In order to use astir you need to have its binaries present at your computer. Astir has been written in C++ and as such it needs to be compiled at some point before use.

## Windows
If you are running on a Windows machine, there are three options available to you. You can

+ download the compiled binaries from the [GitHub Releases](https://github.com/lexected/astir/releases) page of the repository,
+ clone the source code and compile it refering to the native Visual Studio solution,
    ```git
    git clone --recursive URL https://github.com/lexected/astir
    ```
+ clone the source code, CMake, and then compile the sources.
    ```bash
    git clone --recursive URL https://github.com/lexected/astir
    cd astir
    ```

## Linux

## MacOSX

## A note on downloading the source code
Astir has one compile-time dependency, [dimcli](https://github.com/gknowles/dimcli), which is included in the repository as a [git submodule](https://git-scm.com/book/en/v2/Git-Tools-Submodules). Hence, in order to have a successful compilation you need to initialize the submodules prior to compiling. That can be done by either specifying `--recursive` when cloning the repository or by running the following in the local copy

```git
git submodule init
git submodule update
```