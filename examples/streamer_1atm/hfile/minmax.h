
double MAX2(double a,double b){

double max;

  if(a>=b)max=a;
  else max=b;

  return max;
}

double MAX3(double a,double b,double c){

double max;

  if(a<b)max=b;
  else max=a;

  if(max>c);
  else max=c;

  return max;
}

double MAX5(double a,double b,double c,double d,double e){

double max;

  if(a<b)max=b;
  else max=a;

  if(max>c);
  else max=c;

  if(max>d);
  else max=d;

  if(max>e);
  else max=e;

  return max;
}

double MIN2(double a,double b){

double min;

  if(a<=b)min=a;
  else min=b;

  return min;
}

double MIN3(double a,double b,double c){

double min;

  if(a<b)min=a;
  else min=b;

  if(min<c);
  else min=c;

  return min;
}

double MIN5(double a,double b,double c,double d,double e){

double min;

  if(a<b)min=a;
  else min=b;

  if(min<c);
  else min=c;

  if(min<d);
  else min=d;

  if(min<e);
  else min=e;

  return min;
}
