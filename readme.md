## [m32-pars-adapt](https://github.com/matgat/m32-pars-adapt.git)
[![linux-build](https://github.com/matgat/m32-pars-adapt/actions/workflows/linux-build.yml/badge.svg)](https://github.com/matgat/m32-pars-adapt/actions/workflows/linux-build.yml)
[![ms-build](https://github.com/matgat/m32-pars-adapt/actions/workflows/ms-build.yml/badge.svg)](https://github.com/matgat/m32-pars-adapt/actions/workflows/ms-build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

A tool to help the parametrization of m32 based machines.
The main functions are:

* Update an old `MachSettings.udt` file using a newer one as template
* Adapt a `MachSettings.udt` or `par2kax.txt` to a given machine using a parameters database

_________________________________________________________________________
## Requirements

On windows:
* C++ runtime [`VC_redist.x64.exe`](https://aka.ms/vs/17/release/vc_redist.x64.exe).
* Manual comparisons invoke [WinMerge](https://winmerge.org)
* Parsing errors invoke [Notepad++](https://notepad-plus-plus.org)

On linux:
* Manual comparisons invoke [Meld](https://meldmerge.org)
* Parsing errors invoke [mousepad](https://docs.xfce.org/apps/mousepad/start)


_________________________________________________________________________
## Usage

To print usage info:

```bat
> m32-pars-adapt --help
```

To update an old `MachSettings.udt` transferring its values to a newer one:

```bat
> m32-pars-adapt --tgt new\MachSettings.udt --db old\MachSettings.udt
```

To adapt a `MachSettings.udt` for a certain machine:

```bat
> cd %UserProfile%\Macotec\Machines\m32-Strato\sde\userdata
> m32-pars-adapt --tgt MachSettings.udt --db configs\machsettings-overlays.txt --mach WR-4.9/4.6
```

To adapt a Sipro axes parameter file for a certain machine:

```bat
> cd %UserProfile%\Macotec\Machines\m32-Strato\sde\param
> m32-pars-adapt --tgt par2kax.txt --db par2kax-overlays.txt --mach HP-6.0/4.6-(lowe,fast)
```

Normally no file will be overwritten: the program will create a temporary
file that will be automatically deleted after a manual merge.

> [!NOTE]
> With the option `--quiet` the manual merge will be skipped, and if
> no output file was specified with `--out` the adapted file will
> replace the original after a backup copy in the same directory

### Exit values

| Return value | Meaning                                |
|--------------|----------------------------------------|
|      0       | Operation successful                   |
|      1       | Operation completed but with issues    |
|      2       | Operation aborted due to a fatal error |



_________________________________________________________________________
## Updating a `udt` file
A `udt` file is a series of fields like these:

    var_name = value # comment 'var_label'

This program eases the operation to update an old file to a newer
one recognizing the fields and transferring the old values to the
newer file, that acts as a template.

> [!NOTE]
> Unfortunately this operation cannot be fully automatic and almost
> always needs a manual check of the generated file: it's not trivial
> to deal with possible fields semantic changes.



_________________________________________________________________________
## Adapting a file
Given a proper database of value overlays (more below), it's possible
to automate the adaptation of both `udt` and `par2kax.txt` files
according to a machine type.


_________________________________________________________________________
### Machine types
It's possible to specify a machine type through the `--mach` command
argument or the predefined field `vaMachName` in `udt` file.
A machine type is identified by a string like:

    <name>-<sizes>-(option1,option2,...)

From the `<name>` part is inferred the machine family.
The recognized ones are:

| *id*  | *Machine*     | *Type*   |
|-------|---------------|----------|
| `STC` |  StarCut      | *Float*  |
|`MSFR` |  MasterFR     | *Float*  |
|`MSFRV`|  MasterFRV    | *Float*  |
| `F`   |  ActiveE/F    | *Strato* |
| `FR`  |  ActiveFR/FRS | *Strato* |
| `W`   |  ActiveW      | *Strato* |
| `WR`  |  ActiveWR     | *Strato* |
| `HP`  |  ActiveHP     | *Strato* |

For *Strato* machines two sizes must be specified:

    <name>-<cut-bridge>/<algn-span>-(option1,option2,...)

For example:

    ActiveFRS-4.0/3.2

The recognized values:

	F: cut-bridge : 3.7, 4.6
	F: algn-span  : 3.2

	W,WR,HP,FR: cut-bridge : 4.0, 4.9, 6.0
	W,WR,HP,FR: algn-span  : 3.2, 4.6

> [!WARNING]
> An unrecognized value will cause an error.

Finally, a list of arbitrary comma separated options can be specified.

> [!TIP]
> Typical options for *Strato* machines are
> `opp`, `lowe`, `rot`, `buf`, `no-buf`
> but new arbitrary strings can be added.

Some valid machine strings:

    ActiveWR-4.9/4.6
    actF-3.7/3.2-(buf,opp)
    StarCut-9.0-(lowe)
    MsFR



_________________________________________________________________________
### Parameters database
The parameters database consists of a text file containing
an extended/simplified json-like syntax.

_________________________________________________________________________
#### Syntax
* Key names can be unquoted (double quotes necessary in case of spaces or other special chars)
* New line acts as a key-value separator (in that case other separators like comma are optional)
* Supported multiple (comma separated) keys
* Equal sign is tolerated for plain `key=value` assignments
* Supported double slash line comments (`//`) as shown in the example below

```js
"key1", key2 :
   {// Subchilds comment

    "subkey1" : // Key comment
       {// Subchilds comment
        name1: 170 // Value comment
        name2 = 2.4 // Yes, also equal sign
       }

    subkey2 :
       {
        unquoted_key: "quoted value"
        "quoted key": unquoted_value
       }
   }
```

_________________________________________________________________________
#### Content
First level keys are the machine families (`W`, `WR`, ...)
the recognized second level keys are:

	`common`, `cut-bridge`, `algn-span`, `+<option-name>`

> [!IMPORTANT]
> Second level keys whose name is prefixed with `+`
> represent special groups denoting *options*.
> The values inside these groups are applied last,
> overwriting possible existing fields with the same name.
> Adapting a `udt` file that has already superimposed
> options can lead to incoherences, so the program will
> try to detect this case through the value of `vaMachName`.



_________________________________________________________________________
#### DB structure for MachSettings.udt
Here is the expected database structure used to adapt
a `MachSettings.udt` file:

```
┐
├mach┐
│    ├"common"-{nam=val,...}
│    ├"cut-bridge"
│    │  ├"dim"-{nam=val,...}
│    │  ├"dim"-{nam=val,...}
│    │  └···
│    ├"algn-span"
│    │ ├"dim"-{nam=val,...}
│    │ ├"dim"-{nam=val,...}
│    │ └···
│    ├"+option"-{nam=val,...}
│    └···
└···
```

Example:

```
WR,HP :
   {
    "common" :
       {
        vqProbe1_DX: 0.1
       }

    "cut-bridge" :
       {
        "6.0" :
           {
            vnAlgnBlks_N: 8
           }
       }

    "algn-span" :
       {
        "3.2" :
           {
            vqX_AlgnTableEnd: 3780
           }

        "4.6" :
           {
            vqX_AlgnTableEnd: 5280
           }
       }
   }

HP :
   {
    "common" :
       {
        vnMach_Type: 11
       }

    "+fast" :
       {// Velocizzazioni taglio
        vqBlade_SpdMax: 160000
       }
   }
```


_________________________________________________________________________
#### DB structure for par2kax.txt
Here is the expected database structure used to adapt
a `par2kax.txt` file:

```
┐
├mach┐
│    ├"common"┐
│    │        ├"ax"-{nam=val,...}
│    │        ├"ax"-{nam=val,...}
│    │        └···
│    ├"cut-bridge"┐
│    │            ├"dim"┐
│    │            │     ├"ax"-{nam=val,...}
│    │            │     ├"ax"-{nam=val,...}
│    │            │     └···
│    │            ├"dim"┐
│    │            │     ├"ax"-{nam=val,...}
│    │            │     ├"ax"-{nam=val,...}
│    │            │     └···
│    │            └···
│    ├"algn-span"┐
│    │           ├"dim"┐
│    │           │     ├"ax"-{nam=val,...}
│    │           │     ├"ax"-{nam=val,...}
│    │           │     └···
│    │           ├"dim"┐
│    │           │     ├"ax"-{nam=val,...}
│    │           │     ├"ax"-{nam=val,...}
│    │           │     └···
│    │           └···
│    ├"+option"┐
│    │         ├"ax"-{nam=val,...}
│    │         ├"ax"-{nam=val,...}
│    │         └···
│    └···
└···
```

Example:

```
W,WR,HP :
   {
    "cut-bridge" :
       {
        "4.0" :
           {
            "Ysup", "Yinf" :
               {
                MaxPos = 4282
               }
           }
       }

    "+opp" :
       {
        "Xr", "Ysup", "Yinf", "Zg" :
           {
            InvDir = 1
            InvEnc = 1
           }
       }
   }

HP :
   {
    "common" :
       {
        "Xs" :
           {
            MmRif = 1.45646391 // 1/86.28 40pi
           }
       }
   }
```

_________________________________________________________________________
## Build
You need a `c++23` compliant toolchain.
Check the operations in the python script:

```sh
$ git clone https://github.com/matgat/m32-pars-adapt.git
$ cd m32-pars-adapt
$ python build/build.py
```

To run tests:

```sh
$ python test/run-all-tests.py
```


### linux
Launch `make` directly:

```sh
$ cd build
$ make
```

To run unit tests:

```sh
$ make test
```

> [!TIP]
> If building a version that needs `{fmt}`,
> install the dependency beforehand with
> your package manager:
>
> ```sh
> $ sudo pacman -S fmt
> ```
>
> or
>
> ```sh
> $ sudo apt install -y libfmt-dev
> ```


### Windows

On Windows you need Microsoft Visual Studio 2022 (Community Edition).
Once you have `msbuild` visible in path, you can launch the build from the command line:

```bat
> msbuild build/m32-pars-adapt.vcxproj -t:Rebuild -p:Configuration=Release -p:Platform=x64
```

> [!TIP]
> If building a version that needs `{fmt}`
> install the dependency beforehand with `vcpkg`:
>
> ```bat
> > git clone https://github.com/Microsoft/vcpkg.git
> > cd .\vcpkg
> > .\bootstrap-vcpkg.bat -disableMetrics
> > .\vcpkg integrate install
> > .\vcpkg install fmt:x64-windows
> ```
>
> To just update the `vcpkg` libraries:
>
> ```bat
> > cd .\vcpkg
> > git pull
> > .\bootstrap-vcpkg.bat -disableMetrics
> > .\vcpkg upgrade --no-dry-run
> ```
