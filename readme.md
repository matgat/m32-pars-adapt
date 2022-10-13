## [m32-pars-adapt](https://github.com/matgat/m32-pars-adapt.git)

A tool to help the proper parametrization of m32 based machines,
using a json-like database that collects the proper parameters for the
various machine types.



_________________________________________________________________________
## Usage
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
> m32-pars-adapt --tgt par2kax.txt --db par2kax-overlays.txt --machine StratoHP-6.0/4.6
```

Normally no file will be overwritten: the program will create a temporary
file that will be automatically deleted after a manual merge.

When the option `--quiet` is used, the file will be backupped
in the same directory and then overwritten without the user
intervention.

If the output file name is specified with `--out`, there won't
be any filesystem actions (deleting or substitution).



_________________________________________________________________________
## Requirements
In Windows for the manual merging is performed
invoking the external utility *WinMerge*.
The program will try the following paths:
```
"C:\Macotec\Apps\WinMerge\WinMergeU.exe"
"%PROGRAMFILES%\WinMerge\WinMergeU.exe"
```
And as fallback the association with the extension `.WinMerge`



_________________________________________________________________________
## Parameters database
### Syntax
The format is an extended/simplified json syntax:
* Key names can be unquoted, double quotes are necessary just in case of special chars
* Supported multiple keys, comma separated
* No comma or semicolons necessary to separate blocks
* Equal sign is tolerated for plain `key=value` assignments
* Supported double slash line comments (`//`) at block start and after values

```js
"key1", key2 :
   {// Subchilds comment

    "subkey1" :
       {// Subchilds comment
        name1: 170 // Value comment
        name2 = 2.4 // Yes, also equal sign
       }

    subkey2 :
       {// Subchilds comment
        name3: "a quoted value" // Value comment
        name4: unquoted // Value comment
       }
   }
```

_________________________________________________________________________
### Structure for MachSettings.udt
Here is the expected database structure used to adapt
a `MachSettings.udt` file:
```
root┐
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
### Structure for par2kax.txt
Here is the expected database structure used to adapt
a `par2kax.txt` file:
```
root┐
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
