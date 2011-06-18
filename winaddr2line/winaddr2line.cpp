// winaddr2line.cpp : Defines the entry point for the console application.
//

//#undef UNICODE
//#undef _UNICODE
#ifdef UNICODE
#define DBGHELP_TRANSLATE_TCHAR
#endif

#include "stdafx.h"
#include <windows.h>
#include "dbghelp.h"

#define	VERSION				TEXT("1.0.0")

#define OPT_ADDRESSES		TEXT("-a")
#define OPT_ADDRESSES_L		TEXT("--address")
#define OPT_PRETTY_PRINT	TEXT("-p")
#define OPT_PRETTY_PRINT_L	TEXT("--pretty-print")
#define OPT_BASENAME		TEXT("-s")
#define OPT_BASENAME_L		TEXT("--basename")
#define OPT_EXE				TEXT("-e")
#define OPT_EXE_L			TEXT("--exe")
#define OPT_FUNCTIONS		TEXT("-f")
#define OPT_FUNCTIONS_L		TEXT("--functions")
#define OPT_DEMANGLE		TEXT("-C")
#define OPT_DEMANGLE_L		TEXT("--demangle")
#define OPT_HELP			TEXT("-h")
#define OPT_HELP_L			TEXT("--help")
#define OPT_VERSION			TEXT("-v")
#define OPT_VERSION_L		TEXT("--version")
#define OPT_SYMBOL_PATH		TEXT("-y")
#define OPT_SYMBOL_PATH_L	TEXT("--symbol-path")

struct option 
{
	BOOL addresses; 
	BOOL pretty_print;
	BOOL basename;
	BOOL functions;
	BOOL demangle;
	BOOL help;
	BOOL version;
	PTSTR exe;
	PTSTR symbol_path;
	DWORD64 func_addr;

	option() : addresses(FALSE), pretty_print(FALSE),
		basename(FALSE), functions(FALSE), demangle(FALSE),
		help(FALSE), version(FALSE), func_addr(0),
		exe(TEXT("a.exe")), symbol_path(NULL)
	{
	}
};

struct symbol_info
{
	PSYMBOL_INFO symbol;
	PIMAGEHLP_LINE64 line;
	BOOL success;

	symbol_info()
	{
		symbol = reinterpret_cast<PSYMBOL_INFO>(new char[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)]);
		memset(symbol, 0, sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR));
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = MAX_SYM_NAME;
		line = new IMAGEHLP_LINE64();
		memset(line, 0, sizeof(IMAGEHLP_LINE64));
		line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		success = TRUE;
	}

	~symbol_info()
	{
		delete symbol;
		delete line;
	}
};

void print_version()
{
	_tprintf(TEXT("winaddr2line %s\r\n"), VERSION);
}

void print_help()
{
	printf("%s\r\n", "Usage: winaddr2line [option(s)] [addr(s)]\n"
 " Convert addresses into line number/file name pairs.\n"
 " If no addresses are specified on the command line, they will be read from stdin\n"
 " The options are:\n"
  "  -a --addresses         Show addresses\n"
  "  -e --exe=<executable>  Set the input file name (default is a.exe)\n"
  "  -p --pretty-print      Make the output easier to read for humans\n"
  "  -s --basenames         Strip directory names\n"
  "  -f --functions         Show function names\n"
  "  -C --demangle[=style]  Demangle function names\n"
  "  -y --symbol-path=<direcoty_to_symbol>    (option specific to winaddr2line)\n"
  "                         Set the directory to search for symbol file (.pdb)\n"
  "  -h --help              Display this information\n"
  "  -v --version           Display the program's version\n");
}

int parse_option(int argc, TCHAR* argv[], option& opt)
{
	if(argc < 2)
	{
		print_help();
		return 1;
	}
	for(int i = 0; i < argc; ++i)
	{
		if(0 == _tcsncmp(OPT_ADDRESSES, argv[i], _tcslen(OPT_ADDRESSES))
			|| 0 == _tcsncmp(OPT_ADDRESSES_L, argv[i], _tcslen(OPT_ADDRESSES_L)))
		{
			opt.addresses = TRUE;
			continue;
		}
		if(0 == _tcsncmp(OPT_PRETTY_PRINT, argv[i], _tcslen(OPT_PRETTY_PRINT))
			|| 0 == _tcsncmp(OPT_PRETTY_PRINT_L, argv[i], _tcslen(OPT_PRETTY_PRINT_L)))
		{
			opt.pretty_print = TRUE;
			continue;
		}
		if(0 == _tcsncmp(OPT_BASENAME, argv[i], _tcslen(OPT_BASENAME))
			|| 0 == _tcsncmp(OPT_BASENAME_L, argv[i], _tcslen(OPT_BASENAME_L)))
		{
			opt.basename = TRUE;
			continue;
		}
		if(0 == _tcsncmp(OPT_FUNCTIONS, argv[i], _tcslen(OPT_FUNCTIONS))
			|| 0 == _tcsncmp(OPT_FUNCTIONS_L, argv[i], _tcslen(OPT_FUNCTIONS_L)))
		{
			opt.functions = TRUE;
			continue;
		}
		if(0 == _tcsncmp(OPT_DEMANGLE, argv[i], _tcslen(OPT_DEMANGLE))
			|| 0 == _tcsncmp(OPT_DEMANGLE_L, argv[i], _tcslen(OPT_DEMANGLE_L)))
		{
			opt.demangle = TRUE;
			continue;
		}
		if(0 == _tcsncmp(OPT_HELP, argv[i], _tcslen(OPT_HELP))
			|| 0 == _tcsncmp(OPT_HELP_L, argv[i], _tcslen(OPT_HELP_L)))
		{
			opt.help = TRUE;
			print_help();
			return 1;
		}
		if(0 == _tcsncmp(OPT_VERSION, argv[i], _tcslen(OPT_VERSION))
			|| 0 == _tcsncmp(OPT_VERSION_L, argv[i], _tcslen(OPT_VERSION_L)))
		{
			opt.version = TRUE;
			print_version();
			return 1;
		}
		if(0 == _tcsncmp(OPT_EXE, argv[i], _tcslen(OPT_EXE))
			|| 0 == _tcsncmp(OPT_EXE_L, argv[i], _tcslen(OPT_EXE_L)))
		{		
			if(i + 1 >= argc)
			{
				_tprintf(TEXT("option '%s' requires an argument\r\n"), argv[i]);
				return 1;
			}	
			++i;
			opt.exe = argv[i];
			continue;
		}
		if(0 == _tcsncmp(OPT_SYMBOL_PATH, argv[i], _tcslen(OPT_SYMBOL_PATH))
			|| 0 == _tcsncmp(OPT_SYMBOL_PATH_L, argv[i], _tcslen(OPT_SYMBOL_PATH_L)))
		{		
			if(i + 1 >= argc)
			{
				_tprintf(TEXT("option '%s' requires an argument\r\n"), argv[i]);
				return 1;
			}	
			++i;
			opt.symbol_path = argv[i];
			continue;
		}
		// if argv isn't a known argument, it's assumed to be address in hex format
		opt.func_addr = _tcstoul(argv[i], NULL, 16);
	}
	return 0;
}

void print(option &opt, symbol_info &sym)
{
	if(!sym.success)
	{
		if(opt.functions)
			_tprintf(TEXT("??\r\n"), opt.func_addr);
		_tprintf(TEXT("??:0\r\n"));

		return;
	}
	if(opt.pretty_print)
	{
		if(opt.addresses)
			_tprintf(TEXT("0x%08x: "), opt.func_addr);

		if(opt.functions)
		{
			_tprintf(TEXT("%s at "), sym.symbol->Name);
		}

		if(opt.basename)
		{
			TCHAR fname[_MAX_FNAME] = {0}, ext[_MAX_EXT] = {0};
			_tsplitpath_s(sym.line->FileName, NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT);
			_tprintf(TEXT("%s%s:%d\r\n"), fname, ext, sym.line->LineNumber);
		}
		else
			_tprintf(TEXT("%s:%d\r\n"), sym.line->FileName, sym.line->LineNumber);
	} // if(opt.pretty_print) 
	else
	{
		if(opt.addresses)
			_tprintf(TEXT("0x%08x\r\n"), opt.func_addr);

		if(opt.functions)
		{
			_tprintf(TEXT("%s\r\n"), sym.symbol->Name);
		}

		if(opt.basename)
		{
			TCHAR fname[_MAX_FNAME] = {0}, ext[_MAX_EXT] = {0};
			_tsplitpath_s(sym.line->FileName, NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT);
			_tprintf(TEXT("%s%s:%d\r\n"), fname, ext, sym.line->LineNumber);
		}
		else
			_tprintf(TEXT("%s:%d\r\n"), sym.line->FileName, sym.line->LineNumber);
	} // if(opt.pretty_print) 
}

int addr2line(option &opt)
{
	HANDLE proc;
	symbol_info result;

	proc = GetCurrentProcess();

	DWORD sym_opt = SymGetOptions();
	if(opt.demangle)
		sym_opt |= SYMOPT_UNDNAME;
	else
		sym_opt &= ~SYMOPT_UNDNAME;
	SymSetOptions(sym_opt);

	if (!SymInitialize(proc, opt.symbol_path, FALSE))
	{
		return GetLastError();
	}

	DWORD64 base_addr = 0;
	DWORD64 load_addr = 0;

	load_addr = SymLoadModuleEx(proc,    // target process 
		NULL,        // handle to image - not used
		opt.exe,	 // name of image file
		NULL,        // name of module - not required
		base_addr,  // base address - not required
		0,           // size of image - not required
		NULL,        // MODLOAD_DATA used for special cases 
		0);          // flags - not required
	if(0 == load_addr)
	{
		return GetLastError();
	}

	if(opt.func_addr != 0)
	{ // if function address isn't given as argument, read from stdio
		result.success = TRUE;

		if(opt.functions)
		{
			DWORD64  displacement = 0;
			if (0 == SymFromAddr(proc, opt.func_addr, &displacement, result.symbol))
			{
				result.success = FALSE;
			}
		}

		DWORD displacement;
		if (0 == SymGetLineFromAddr64(proc, opt.func_addr, &displacement, result.line))
		{
			result.success = FALSE;
		}

		print(opt, result);
	}
	SymUnloadModule64(proc, load_addr);
	SymCleanup(proc);
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	option opt;
	if(0 == parse_option(argc, argv, opt))
		return addr2line(opt);
	return 0;
}
