#pragma once
#include <cmath>
#include <algorithm>
#include <vector>
#include <ctime>
#include <random>
#include <iostream>
using namespace std;
const double PI = acos(-1.0);
struct WxzRandom
{
	WxzRandom()
	{
		generator = default_random_engine(rd());
		dis = uniform_real_distribution<double>(0.0, 1.0);
	}
	double C11Rand()
	{
		return dis(generator);
	}
	default_random_engine generator;
	random_device rd;
	uniform_real_distribution<double> dis;
};
WxzRandom myRandom;
struct Vec3
{
	double x, y, z;
	float fx()
	{
		return float(x);
	}
	float fy()
	{
		return float(y);
	}
	float fz()
	{
		return float(z);
	}
	Vec3()
	{

	}
	Vec3(double X, double Y, double Z)
	{
		x = X;
		y = Y;
		z = Z;
	}
	double length()
	{
		return sqrt(x*x + y*y + z*z);
	}
	double sqrlength()
	{
		return x*x + y*y + z*z;
	}
	Vec3 normalize()
	{
		double inv = (1 / (this->length()));
		return Vec3(inv*x, inv*y, inv*z);
	}
	Vec3 negate()
	{
		return Vec3(-x, -y, -z);
	}
	Vec3 operator + (const Vec3 b)
	{
		return Vec3(x + b.x, y + b.y, z + b.z);
	}
	Vec3 operator - (const Vec3 b)
	{
		return Vec3(x - b.x, y - b.y, z - b.z);
	}
	friend Vec3 operator * (double a, Vec3 my)
	{
		return Vec3(a*my.x, a*my.y, a*my.z);
	}
	friend Vec3 operator * (Vec3 my, double a)
	{
		return Vec3(a*my.x, a*my.y, a*my.z);
	}
	Vec3 operator /(const double a)
	{
		return Vec3(x / a, y / a, z / a);
	}
	Vec3 operator ^ (Vec3 v)
	{
		return Vec3(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
	}
	double dot(Vec3 v)
	{
		return x*v.x + y*v.y + z*v.z;
	}
	void operator +=(Vec3 v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		//return *this;
	}
};