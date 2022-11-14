int year;
int month;
int day;

f(n){
	if(n){
		return f(n-1) + n;
	}else{
		return n;
	}
}

g(from, to, step){
	int sum;
	sum = 0;
	while(SQAless(from, to+1)){
		sum = sum + from;
		from = from + step;
	}
	return sum;
}

printStrs(strs){
	int i;
	int s;

	i = 0;
	s = SQAload(strs, i*4);
	while(s){
		puts(s);
		i = i + 1;
		s = SQAload(strs, i*4);
	}
}

main(argc, argv, env){
	int e;
	int base;
	int i;
	int val;

	base = malloc(40);
	
	i = 0;
	while(SQAless(i, 10)){
		SQAstore(base, i*4, i);
		i = i + 1;
	}

	i = 0;
	while(SQAless(i, 10)){
		val = SQAload(base, i*4);
		output(val);
		i = i + 1;
	}

	val = 97;
	i = 0;
	while(SQAless(i, 26)){
		SQAstore(base, i, val + i);
		i = i + 1;
	}
	SQAstore(base, 26, 0);
	puts(base);
	free(base);
	
	printStrs(argv);

	printStrs(env);

	year = 2022;
	month = 11;
	day = 14;
	int s;
	s = f(100);
	output(s);
	s = g(1, 100, 1);
	output(s);
	output(year);
	output(month);
	output(day);
	output(argc);

	int pid;
	pid = fork();
	if(SQAless(pid, 0)){
		output(pid);
	}
	else if(SQAequal(pid, 0)){
		output(pid);
	}
	else{
		wait(0);
		output(pid);
	}

	return 0;
}

