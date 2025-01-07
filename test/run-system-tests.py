#!/usr/bin/env python
import os, sys
import subprocess
import inspect
import time
import tempfile
import difflib
import re

# 🧬 Settings
script_dir = sys.path[0]
projectname = os.path.basename(os.path.dirname(script_dir))
if os.name=='nt':
    exe = f"../build/bin/win-x64-Release/{projectname}.exe"
else:
    exe = f"../build/bin/{projectname}"




#----------------------------------------------------------------------------
GRAY = '\033[90m';
RED = '\033[91m';
GREEN = '\033[92m';
YELLOW = '\033[93m';
BLUE = '\033[94m';
MAGENTA = '\033[95m';
CYAN = '\033[96m';
END = '\033[0m';

#----------------------------------------------------------------------------
def set_title(title):
    if os.name=='nt':
        os.system(f"title {title}")
    else:
        sys.stdout.write(f"\x1b]2;{title}\x07")

#----------------------------------------------------------------------------
def is_temp_console():
    return os.name == 'nt' # Sketchy, but avoids psutil dependency

#----------------------------------------------------------------------------
def closing_bad(msg):
    print(f"\n{RED}{msg}{END}")
    if is_temp_console():
        input(f'{YELLOW}Press <ENTER> to exit{END}')

#----------------------------------------------------------------------------
def closing_ok(msg):
    print(f"\n{GREEN}{msg}{END}")
    if is_temp_console():
        print(f'{GRAY}Closing...{END}')
        time.sleep(3)

#----------------------------------------------------------------------------
def is_manual_mode():
    return len(sys.argv)>1 and sys.argv[1].lower().startswith('man');

#----------------------------------------------------------------------------
def wait_for_keypress():
    if os.name=='nt':
        import msvcrt
        key = chr(msvcrt.getch()[0])
    else:
        import termios
        fd = sys.stdin.fileno()
        oldterm = termios.tcgetattr(fd)
        newattr = termios.tcgetattr(fd)
        newattr[3] = newattr[3] & ~termios.ICANON & ~termios.ECHO
        termios.tcsetattr(fd, termios.TCSANOW, newattr)
        try: key = sys.stdin.read(1)[0]
        except IOError: pass
        finally: termios.tcsetattr(fd, termios.TCSAFLUSH, oldterm)
    return key

#----------------------------------------------------------------------------
def launch(command_and_args):
    print(f"{GRAY}", end='', flush=True)
    start_time_s = time.perf_counter() # time.process_time()
    return_code = subprocess.call(command_and_args)
    exec_time_s = time.perf_counter() - start_time_s # time.process_time()
    exec_time_ms = 1000.0 * exec_time_s
    if exec_time_s>0.5:
        exec_time_str = f"{CYAN}{exec_time_s:.2f}{END}s"
    else:
        exec_time_str = f"{CYAN}{exec_time_ms:.2f}{END}ms"
    print(f"{END}{command_and_args[0]} returned: {GREEN if return_code==0 else RED}{return_code}{END} after {exec_time_str}")
    return return_code, exec_time_ms

#----------------------------------------------------------------------------
def create_text_file(dir, fname, text_content, encoding):
    file_path = os.path.join(dir, fname)
    try:
        with open(file_path, 'wb') as f:
            f.write( text_content.encode(encoding) )
        return file_path
    except Exception as e:
        print(f'{RED}Cannot create file: {END}{file_path}')
        print(f'{RED}{e}{END}')

#----------------------------------------------------------------------------
class TextFile:
    def __init__(self, file_name=None, content=None, encoding='utf-8'):
        self.file_name = file_name
        self.content = content.replace('#file_name#', file_name)
        self.encoding = encoding
    def create_in(self, dir):
        return create_text_file(dir, self.file_name, self.content, self.encoding)
    def create_with_content_in(self, dir, provided_content):
        return create_text_file(dir, self.file_name, provided_content, self.encoding)

#----------------------------------------------------------------------------
class TemporaryPath:
    def __init__(self, fname):
        self.path = os.path.join(tempfile.gettempdir(), fname)
    def __enter__(self):
        return self
    def __exit__(self, exc_type, exc_val, exc_tb):
        try: os.remove(self.path)
        except: pass

#----------------------------------------------------------------------------
def check_strings_equality(str1, str2):
    def show_char(ch):
        return ch if ch.isprintable() and not ch.isspace() else repr(ch)[1:-1]
    if str1 == str2: return True
    result = []
    diff = difflib.ndiff(str1, str2)
    for s in diff:
        if s[0] == ' ':
            result.append(GRAY + s[-1] + END) # equal parts
        elif s[0] == '-': # missing characters
            result.append(RED + show_char(s[-1]) + END)
        elif s[0] == '+': # surplus characters
            result.append(GREEN + show_char(s[-1]) + END)
    print(''.join(result))
    return False

#----------------------------------------------------------------------------
def have_same_content(path1, path2):
    equal = False
    if not os.path.isfile(path1):
        print(f'{RED}{path1} {MAGENTA}not existing{END}')
    elif not os.path.isfile(path2):
        print(f'{RED}{path2} {MAGENTA}not existing{END}')
    else:
        try:
            with open(path1, 'r') as file1, open(path2, 'r') as file2:
                equal = check_strings_equality(file1.read(), file2.read())
            if is_manual_mode() and not equal: show_text_files_comparison(path1, path2)
        except Exception as e:
            print(f'{RED}{e}{END}')
    return equal

#----------------------------------------------------------------------------
def show_text_file(file_path):
    print(f"opening {file_path}")
    subprocess.run(['start' if os.name=='nt' else 'xdg-open', file_path])

#----------------------------------------------------------------------------
def show_text_files_comparison(left_file, middle_file, right_file =""):
    if os.name=='nt':
        # -r :recurse subfolders
        # -e :close WinMerge with a single Esc key press
        # -f :filter to restrict the comparison
        # -x :closes in case of identical files
        # -s :limits WinMerge windows to a single instance
        # -u :prevents WinMerge from adding paths to the Most Recently Used (MRU) list
        # -wl -wr :opens the side as read-only
        # -dl -dr :descriptions
        command_and_args = [os.path.expandvars("%ProgramFiles%/WinMerge/WinMergeU.exe"), "-s", "-e", "-u", "-r", "-f", "*.*", "-x", "-wl", left_file, middle_file]
        if right_file: command_and_args.append(right_file)
    else:
        command_and_args = ["meld", left_file, middle_file]
        if right_file: command_and_args.append(right_file)
    subprocess.call(command_and_args)

#----------------------------------------------------------------------------
def get_test_file_path(fname):
    return os.path.join(script_dir, "testfiles", fname)

#----------------------------------------------------------------------------
class Test:
    keys = iter("1234567890qwertyuiopasdfghjklzxcvbnm")
    def __init__(self, title, action):
        try:
            self.menu_key = next(self.keys)
        except StopIteration:
            print(f'{RED}No more keys available for test menu!{END}')
            return
        self.title = title
        self.action = action
    def run(self):
        return self.action()
#----------------------------------------------------------------------------
class Tests:
    def __init__(self, manual_mode):
        self.list = []
        self.manual_mode = manual_mode
        # Collect the test methods sorted by definition position
        prefix = 'test'
        test_methods = [(name, func) for name, func in inspect.getmembers(self, inspect.ismethod) if name.startswith(prefix)]
        def linenumber_of(m):
            try: return m[1].__func__.__code__.co_firstlineno
            except: return -1
        test_methods.sort(key=linenumber_of)
        prefix_auto = f'{prefix}_'
        prefix_man = f'{prefix}man_'
        for name, func in test_methods:
            if name.startswith(prefix_auto):
                self.list.append(Test(name[len(prefix_auto):], func))
            elif manual_mode and name.startswith(prefix_man) :
                self.list.append(Test(name[len(prefix_man):], func))


    #========================================================================
    def test_no_args(self):
        ret_code, ms = launch([exe, "-v" if self.manual_mode else "-q"])
        return ret_code==2 # no args given should give a fatal error


    #========================================================================
    def test_out_conflict(self):
        udt = TextFile('file.udt', '\n')
        txt = TextFile('file.txt', '\n')
        with tempfile.TemporaryDirectory() as temp_dir:
            udt_path = udt.create_in(temp_dir)
            txt_path = txt.create_in(temp_dir)
            ret_code1, ms1 = launch([exe, "--tgt", udt_path, "--db", txt_path, "--mach", "W-4.9/4.6", "--out", udt_path, "-v" if self.manual_mode else "-q"])
            ret_code2, ms2 = launch([exe, "--tgt", udt_path, "--db", txt_path, "--mach", "W-4.9/4.6", "--out", txt_path, "-v" if self.manual_mode else "-q"])
            return ret_code1==2 and ret_code2==2 # should complain about output conflict


    #========================================================================
    def test_noadapt_with_options(self):
        udt = TextFile('MachSettings.udt',
        "va0 = \"W-4.9/4.6-(opt)\" # Machine 'vaMachName'\n"
        "vn123 = old # Field 'vnField'\n")
        udt_new = TextFile('MachSettings-overlays.txt',
        "W,WR,HP: {\n"
        "    \"+lowe\": { vnField: new }\n"
        "   }\n")
        with tempfile.TemporaryDirectory() as temp_dir:
            udt_path = udt.create_in(temp_dir)
            db_path = udt_new.create_in(temp_dir)
            out_path = os.path.join(temp_dir,"adapted.udt")
            ret_code, ms = launch([exe, "--tgt", udt_path, "--db", db_path, "--mach", "W-4.9/4.6-(lowe)", "--out", out_path, "-v" if self.manual_mode else "-q"])
            return ret_code==2 # should complain about existing options


    #========================================================================
    def test_adapt_parax(self):
        tgt_path = get_test_file_path('par2kax.txt')
        db_path = get_test_file_path('par2kax-overlays.txt')
        mach = 'HP-6.0/4.6-(lowe)'
        exp_path = get_test_file_path('expected-parax-adapted-HP-6.0-4.6-(lowe).txt')
        with TemporaryPath("~adapted.txt") as out:
            ret_code, exec_time_ms = launch([exe, "--tgt", tgt_path, "--db", db_path, "--mach", mach, "--out", out.path, "-v" if self.manual_mode else "-q", "--options", "no-timestamp"])
            return ret_code==0 and have_same_content(out.path,exp_path)


    #========================================================================
    def test_adapt_udt(self):
        tgt_path = get_test_file_path('MachSettings.udt')
        db_path = get_test_file_path('machsettings-overlays.txt')
        mach = 'HP-6.0/4.6-(lowe)'
        exp_path = get_test_file_path('expected-msett-adapted-HP-6.0-4.6-(lowe).udt')
        with TemporaryPath("~adapted.udt") as out:
            ret_code, exec_time_ms = launch([exe, "--tgt", tgt_path, "--db", db_path, "--mach", mach, "--out", out.path, "-v" if self.manual_mode else "-q", "--options", "no-timestamp"])
            return ret_code==0 and have_same_content(out.path,exp_path)


    #========================================================================
    def test_update_udt(self):
        new_path = get_test_file_path('MachSettings.udt')
        old_path = get_test_file_path('MachSettings-old.udt')
        exp_path = get_test_file_path('expected-msett-updated.udt')
        with TemporaryPath("~updated.udt") as out:
            ret_code, exec_time_ms = launch([exe, "--db", old_path, "--tgt", new_path, "--out", out.path, "-v" if self.manual_mode else "-q", "--options", "no-timestamp"])
            return ret_code==0 and have_same_content(out.path,exp_path)



#----------------------------------------------------------------------------
def run_tests(tests_array):
    tests_results = []
    fails = 0
    for test in tests_array:
        print(f'\n-------------- {GRAY}test {YELLOW}{test.title}{END}')
        if test.run() :
            tests_results.append(f'{GREEN}[passed] {test.title}{END}')
        else:
            tests_results.append(f'{RED}[failed] {test.title}{END}')
            fails += 1
    # Print results
    print('\n-------------------------------------')
    for result_entry in tests_results:
        print(result_entry)
    return fails

#----------------------------------------------------------------------------
def ask_what_to_do(tests_array):
    def print_menu(tests_array):
        print("\n____________________________________")
        for test in tests_array:
            print(f"{YELLOW}[{test.menu_key}]{END}{test.title}", end=' ')
        print(f"{YELLOW}[ESC]{END}Quit")
    def print_selected_entry(key,name):
        print(f'{CYAN}{repr(key)} {BLUE}{name}{END}')
    def get_test(key_char, tests_array):
        return [test for test in tests_array if test.menu_key==key_char]

    print_menu(tests_array)
    key_char = wait_for_keypress()
    if selected:=get_test(key_char, tests_array):
        for test in selected:
            print_selected_entry(key_char,test.title)
            if test.run() :
                print(f'{GREEN}[passed] {test.title}{END}')
            else:
                print(f'{RED}[failed] {test.title}{END}')
    else:
        match key_char:
            case '\x1b' | '\x03':
                print_selected_entry(key_char,'Quit')
                return False
            case _:
                print(f"{RED}{repr(key_char)} is not a valid option{END}")
    return True


#----------------------------------------------------------------------------
def main():
    set_title(__file__)
    os.chdir(script_dir)
    if not os.path.isfile(exe):
        closing_bad(f"{exe} not found!")
        sys.exit(1)
    tests = Tests(is_manual_mode());
    if tests.manual_mode:
        print(f'\n{BLUE}Manual tests on {CYAN}{exe}{END}')
        while ask_what_to_do(tests.list): pass
    else:
        print(f'\n{BLUE}System tests on {CYAN}{exe}{END}')
        fails_count = run_tests(tests.list)
        if fails_count>0:
            closing_bad(f"{fails_count} {'test' if fails_count==1 else 'tests'} failed!")
        else:
            closing_ok(f"All {len(tests.list)} tests passed")
        return fails_count

#----------------------------------------------------------------------------
if __name__ == '__main__':
    sys.exit(main())
