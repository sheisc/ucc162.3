int year;

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

main(argc){
	year = 2022;
	int s;
	s = f(100);
	output(s);
	s = g(1, 100, 1);
	output(s);
	output(year);
	output(argc);
	return 0;
}
