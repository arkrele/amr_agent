// 双曲型電極
// Vele : 電極電圧, cr : 先端曲率半径, b : ギャップ長
// r, z : 電圧を求めたい点
// 戻り値 : (r, z)の電圧
double hyperbolic_curve(double Vele, double cr, double b, double r,double z)
{
	// (z/b)2 - (r/a)2 = 1
	// Q : 焦点
	// (r, z)を通る, 焦点Qの双曲線のパラメータ
	// thetaI : 電極, theta : 求めたい点
	double a, a2, b2, Q;
	double thetaI, theta;
	double V;

	a = sqrt(cr*b);

	// 電極内なら電極電圧を返す
	if (pow(z/b, 2) - pow(r/a, 2) >= 1) {
		V = Vele;
	} else if (z == 0) { // 接地なら0を返す
		V = 0;
	} else {
		thetaI = atan2(a, b);
		Q = sqrt(b*b + a*a); // 焦点
		a2 = ((-(r*r + z*z - Q*Q) + sqrt(pow((r*r + z*z - Q*Q), 2) - 4*(-r*r *Q*Q)))/2);
		a2 = sqrt(a2);
		b2 = sqrt(Q*Q - a2*a2);
		theta = atan2(a2, b2);

		V = Vele * (log(1/tan(theta/2)))/(log(1/(tan(thetaI/2))));
	}

	return V;
}
