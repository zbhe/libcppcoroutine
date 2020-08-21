# libcppcoroutine
stackfull coroutine for c++
```cpp
void* Fib(void*)
{
	uint64_t a = 0;
	uint64_t b = 0;
	uint64_t c = 1;
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
	Coroutine* CoFib = new Coroutine(Fib);
	for( int i = 1; i < 100; i++){
		uint64_t* p = (uint64_t*)CoFib->Resume(nullptr);
		cout << *p << " ";
	}
	delete CoFib;
	cout << endl;
	return 0;
}
```
