## [m32-pars-adapt](https://github.com/matgat/m32-pars-adapt.git)

A tool to help the proper parametrization of m32 based machines,
using a json-like database that collects the proper parameters for the
various machine types.



_________________________________________________________________________
## Usage
Windows binary is dynamically linked to Microsoft c++ runtime,
so needs the installation of
[`VC_redist.x64.exe`](https://aka.ms/vs/17/release/vc_redist.x64.exe)
as prerequisite.

To print usage info:
```
> m32-pars-adapt --help
```

To valorize a `MachSettings.udt` given a machine type:
```
> cd %UserProfile%\Macotec\Machines\m32-Strato\sde\userdata
> m32-pars-adapt --tgt MachSettings.udt --db configs\machsettings-overlays.txt --machine StratoWR-4.9/4.6
```

To update an old `MachSettings.udt` to a new one:
```
> m32-pars-adapt --tgt new\MachSettings.udt --db old\MachSettings.udt
```

To valorize a Sipro parameter file:
```
> cd %UserProfile%\Macotec\Machines\m32-Strato\sde\param
> m32-pars-adapt --tgt par2kax.txt --db par2kax-overlays.txt --machine StratoHP-6.0/4.6-(buf-rot,fast)
```

Normally no file will be overwritten: the program will create a temporary
file that will be automatically deleted after a manual merge.

When the option `--quiet` is used, the manual merge will be skipped:
if an output file name is specified with `--out`,
there won't be any actions after its creation, otherwise
the file to be modified will be backupped in the same directory and
then silently overwritten.


_________________________________________________________________________
## Requirements
C++ runtime [`VC_redist.x64.exe`](https://aka.ms/vs/17/release/vc_redist.x64.exe).

The utility [WinMerge](https://winmerge.org) is invoked whenever a manual
comparison of the files is needed.
The program will try the following paths:
```
"C:\Macotec\Apps\WinMerge\WinMergeU.exe"
"%PROGRAMFILES%\WinMerge\WinMergeU.exe"
```
And as fallback the association with the extension `.WinMerge`


_________________________________________________________________________
## Parameters database
The parameters database consists of a text file containing
an extended/simplified json-like syntax.

_________________________________________________________________________
### Syntax
* Key names can be unquoted, double quotes necessary just if name contains special chars
* Separators like comma or semicolons are not strictly necessary
* Supported multiple (comma separated) keys
* Equal sign is tolerated for plain `key=value` assignments
* Supported double slash line comments (`//`) as shown in the example below
* Block comments (`/*...*/`) are *deliberately* not supported

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
        name3: "a quoted value"
        name4: unquoted
       }
   }
```

_________________________________________________________________________
### Content
First level keys are the machine type:

| *id*  | *Machine*  | *Type*    |
|-------|------------|-----------|
| `STC` |  StarCut   |  *Float*  |
| `FR`  |  MasterFR  |  *Float*  |
| `FRV` |  MasterFRV |  *Float*  |
| `S`   |  ActiveE/F |  *Strato* |
| `W`   |  ActiveW   |  *Strato* |
| `WR`  |  ActiveWR  |  *Strato* |
| `HP`  |  ActiveHP  |  *Strato* |


Recognized second level keys are:

	"common", "cut-bridge", "algn-span", "+<option-name>"

Second level keys whose name is prefixed with `+`
represent special groups denoting *options*.
The values inside these groups are applied last,
overwriting possible existing fields with the same name.
Warning: Adapting a `MachSettings.utd` that has
superimposed options can lead to incoherences,
so in this case the program will rise an error.

Recognized  dimensions:

	S: cut-bridge : 3.7, 4.6
	S: algn-span  : 3.2

	W,WR,HP: cut-bridge : 4.0, 4.9, 6.0
	W,WR,HP: algn-span  : 3.2, 4.6


_________________________________________________________________________
### Structure for MachSettings.udt
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
```js
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
### Structure for par2kax.txt
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
```js
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
```
# pacman -S fmt
$ git clone https://github.com/matgat/m32-pars-adapt.git
$ cd m32-pars-adapt
$ clang++ -std=c++2b -funsigned-char -Wall -Wextra -Wpedantic -Wconversion -O3 -lfmt -o "linux/build/m32-pars-adapt" "source/m32-pars-adapt.cpp"
```

On Windows, use the latest Microsoft Visual Studio Community.
From the command line, something like:
```
> msbuild .msvc/m32-pars-adapt.vcxproj -t:m32-pars-adapt -p:Configuration=Release
```
This project depends on `{fmt}` library, use `vcpkg` to install it:
```
> git clone https://github.com/Microsoft/vcpkg.git
> .\vcpkg\bootstrap-vcpkg.bat -disableMetrics
> .\vcpkg\vcpkg integrate install
> .\vcpkg\vcpkg install fmt:x64-windows
```
