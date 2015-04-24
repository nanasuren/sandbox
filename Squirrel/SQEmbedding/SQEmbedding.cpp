// SQEmbedding.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include <stdarg.h>

#include <sqrat.h>
#include <sqratimport.h>

#include <atlstr.h>

bool compileScript() {
	Sqrat::Script script;
	Sqrat::string errmsg;

	printf("compile start\n");

	if(!script.CompileFile(_T("./Script/test.nut"), errmsg)) {
		printf("failed. compile error [%ls]\n", errmsg.c_str());
	}

	printf("compile successed\n");

	script.Run();

	return true;
}

// 文字列出力関数
static void printfunc(HSQUIRRELVM vm, const SQChar* format, ...)
{
    va_list args;
    va_start(args, format);
#ifdef SQUNICODE
    vwprintf(format, args);
//	wprintf(_T("\n"));
#else
    vprintf(format, args);
//	printf(_T("\n"));
#endif
    va_end(args);
}

int _tmain(int argc, _TCHAR* argv[])
{
	_tsetlocale(LC_ALL, _T(""));
//	_tsetlocale(LC_ALL, _T("Japanese_Japan.932"));

	HSQUIRRELVM vm = sq_open(1024);
	// error handle
	sqstd_seterrorhandlers(vm);

	// this is the global scope where the libraries are registered in(the global table)
//	sq_pushroottable(vm);

	// io/lib
	sqstd_register_iolib(vm);

	// print func
	sq_setprintfunc(vm, printfunc, printfunc);

	// import lib
	sqrat_register_importlib(vm);

	{
		Sqrat::DefaultVM::Set(vm);

		bool isCompile = compileScript();

		while(true) {
			char c = getchar();
			if(c == 'c') {
				isCompile = compileScript();
			}
			else if(c == 'q') {
				break;
			}
			else if(c == 'r') {
				if(isCompile) {
					Sqrat::Array sqInstArray = Sqrat::RootTable(vm).GetSlot(_SC("ActInst"));
					if(sqInstArray.IsNull()) {
						assert(!"invalid");
					}
					for(int i = 0; i < sqInstArray.GetSize(); ++i) {
						Sqrat::Object sqInst = sqInstArray.GetSlot(i);
						// メンバ
						{
							Sqrat::Object sqVariable = sqInst.GetSlot(_SC("name"));
							Sqrat::string name = sqVariable.Cast<Sqrat::string>();
						}

						// 返り値なし関数
						{
							Sqrat::Function sqFunc = Sqrat::Function(sqInst, _SC("trace"));
							if(!sqFunc.IsNull()) {
								sqFunc.Execute();
							}
							else {
								printf("SQ Func : null");
							}
						}

						// 返り値あり関数
						{
							Sqrat::Function sqFunc = Sqrat::Function(sqInst, _SC("getTestArg"));
							if(!sqFunc.IsNull()) {
								Sqrat::SharedPtr<int>& num = sqFunc.Evaluate<int>(10, 20);
								printf("sum = %d\n", *num);
							}
							else {
								printf("SQ Func : null");
							}
						}
					}
				}
				else {
					printf("error compile\n");
				}
			}
		}
	}

	sq_close(vm);

	return 0;
}

