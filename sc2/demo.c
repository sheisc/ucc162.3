int year;

f(n){
	if(n){
		return f(n-1) + n;
	}else{
		return n;
	}
}



main(argc){
	year = 2022;
	int s;
	s = f(100);
	output(s);
	output(year);
	output(argc);
	return 0;
}
