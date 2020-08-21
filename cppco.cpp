#include<stdio.h>
#include<string.h>
#include<stdint.h>
#include<stdlib.h>
#include<strings.h>
#include<assert.h>
#include<vector>
#include<iostream>
using namespace std;
extern "C" void CoSwap(void* From, void* To, void* Arg);
//RDI, RSI, RDX
asm(
	".global CoSwap\n"
	"CoSwap:\n"
	"cmpq %rdi, %rsi\n"
	"je Reset\n"

	"leaq 0x0(%rdi), %rax\n"

	"movq %rax, 0(%rax)\n"
	"movq %rbx, 8(%rax)\n"
	"movq %rcx, 16(%rax)\n"
	"movq %rdx, 24(%rax)\n"
	"movq %rdi, 32(%rax)\n"
	"movq %rsi, 40(%rax)\n"
	"movq %rbp, 48(%rax)\n"
	"movq %rsp, 56(%rax)\n"
	"movq %r8, 64(%rax)\n"
	"movq %r9, 72(%rax)\n"
	"movq %r10, 80(%rax)\n"
	"movq %r11, 88(%rax)\n"
	"movq %r12, 96(%rax)\n"
	"movq %r13, 104(%rax)\n"
	"movq %r14, 112(%rax)\n"
	"movq %r14, 120(%rax)\n"

"Reset:\n"
	"leaq 0x0(%rsi), %rax\n"
	//"movq 0(%rax), %rax\n"
	"movq 8(%rax), %rbx\n"
	"movq 16(%rax), %rcx\n"
	//"movq 24(%rax), %rdx\n"
	//"movq 32(%rax), %rdi\n"
	//"movq 40(%rax), %rsi\n"
	"movq 48(%rax), %rbp\n"
	"movq 56(%rax), %rsp\n"
	//"movq 64(%rax), %r8\n"
	//"movq 72(%rax), %r9\n"
	"movq 80(%rax), %r10\n"
	"movq 88(%rax), %r11\n"
	"movq 96(%rax), %r12\n"
	"movq 104(%rax), %r13\n"
	"movq 112(%rax), %r14\n"
	"movq 120(%rax), %r15\n"

	"movq %rdx, %rdi\n"
	"movq %rdx, %rax\n"
	"ret\n"
);

struct Coroutine{
	using FUNTYPE = void* (*) (void*);
	enum class State{INIT, RUNNING, SUSPEND, DONE};
	void* _Regs[16];//rax, rbx, rcx, rdx, rdi, rsi, rbp, rsp, r8~r15
	void* _StackPtr = nullptr;
	char* _Buf = nullptr;
	State _State = State::INIT;
	static std::vector<Coroutine*> _G;
	static uint32_t Index;

	static Coroutine* Current()
	{
		return _G[Index];
	}
	static void OnReturn()
	{
		volatile void* Ptr = nullptr;
		asm __volatile__("movq %%rax, %0\n" : "=m"(Ptr) : :);
		Coroutine* Cur = _G[Index--];
		Cur->_State = State::DONE;
		Coroutine* Next = _G[Index];
		Next->Resume((void*)Ptr);
	}
	Coroutine(FUNTYPE FPtr, size_t StackSize = 1024*1024)
	{
		bzero(_Regs, sizeof(_Regs));
		if( StackSize > 0){
			_Buf = new char[StackSize];
			_StackPtr = (void*)((char*)_Buf + StackSize);
			_StackPtr = (void*)((uint64_t)_StackPtr & 0xFFFFFFFFFFFFFFF0);
			_StackPtr = (void*)((char*)_StackPtr - 0x10);
			*(uint64_t*)_StackPtr = (uint64_t)OnReturn;
			_StackPtr = (void*)((char*)_StackPtr - 0x8);
			*(uint64_t*)_StackPtr = (uint64_t)FPtr;
		}
		_Regs[7] = _StackPtr;
	}
	~Coroutine()
	{
		if( _Buf != nullptr ){
			assert(_State != State::RUNNING);
			delete [ ] _Buf;
			_Buf = nullptr;
		}
	}
	void* Resume(void* Arg)
	{
		Coroutine* Cur = _G[Index];
		if( Cur->_State != State::DONE ){
			Cur->_State = State::SUSPEND;
		}
		if( this != Cur ){
			_G[++Index] = this;
		}
		if( _State == State::INIT ){
			asm(
				"movq %%rax, 0(%0)\n"
				"movq %%rbx, 8(%0)\n"
				"movq %%rcx, 16(%0)\n"
				"movq %%rdx, 24(%0)\n"
				"movq %%rdi, 32(%0)\n"
				"movq %%rsi, 40(%0)\n"
				"movq %%rbp, 48(%0)\n"
				//"movq %%rsp, 56(%0)\n"
				"movq %%r8, 64(%0)\n"
				"movq %%r9, 72(%0)\n"
				"movq %%r10, 80(%0)\n"
				"movq %%r11, 88(%0)\n"
				"movq %%r12, 96(%0)\n"
				"movq %%r13, 104(%0)\n"
				"movq %%r14, 112(%0)\n"
				"movq %%r14, 120(%0)\n" : :"r"(_Regs):
			);
		}
		_State = State::RUNNING;
		CoSwap(Cur, this, Arg);
		volatile void* Ret = nullptr;
		asm __volatile__("movq %%rax, %0\n" : "=m"(Ret) : :);
		return (void*)Ret;
	}
	void* Yield(void* Arg)
	{
		assert(this == _G[Index]);
		assert(Index != 0 );//cann't yield main
		Coroutine* Pre = _G[--Index];
		_State = State::SUSPEND;
		Pre->_State = State::RUNNING;
		CoSwap(this, Pre, Arg);
		volatile void* Ret = nullptr;
		asm __volatile__("movq %%rax, %0\n" : "=m"(Ret) : :);
		return (void*)Ret;
	}
};
static_assert((uint64_t)&((Coroutine*)(0))->_Regs == 0);
uint32_t Coroutine::Index = 0;
static Coroutine MainCo(nullptr, 0);
std::vector<Coroutine*> Coroutine::_G{1, &MainCo};

///////////////////////////////////////test////////////////////////////
void go(void* Str)
{
	printf("GOGO:%s\n", (const char*)Str);
}
void* Run(void* p)
{
	printf("Run:%p\n", p);
	void* Ret = Coroutine::Current()->Yield((void*)"OK");
	go(Ret);
	return p;
}

void* Fib(void*)
{
	uint64_t a = 1;
	uint64_t b = 1;
	uint64_t c = 1;
	Coroutine::Current()->Yield(&b);
	while(true){
		Coroutine::Current()->Yield(&c);
		a = b;
		b = c;
		c = a + b;
	}
	return nullptr;
}

int main()
{
	Coroutine* Co = new Coroutine(Run);
	cout << "call resume\n";
	const char* Ok = (const char*)Co->Resume(Co);
	cout << "yield back " << Ok << endl;
	void* Ret = Co->Resume((void*)"Done");
	if( Ret == Co ){
		cout << "return back\n";
	}else{
		cout << "something wrong\n";
	}
	delete Co;
	Coroutine* CoFib = new Coroutine(Fib);
	for( int i = 1; i < 100; i++){
		uint64_t* p = (uint64_t*)CoFib->Resume(nullptr);
		cout << *p << " ";
	}
	cout << endl;
	return 0;
}
