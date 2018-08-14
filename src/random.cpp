// $RCSfile$     $Date$     $$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "random.hpp"
#include "utils.h"

#include <boost/version.hpp>
#if BOOST_VERSION >= 104700
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/binomial_distribution.hpp>
#else
#include <boost/random.hpp>
#pragma message("HINT: You are using old version of boost")
#endif

namespace Random
{
	class NormalRand
	{
	public:
		NormalRand()
		{
			_rng.seed(static_cast<unsigned>(time(0)));
		};

		int number(int from, int to);
		//���������� �������������
		double NormalDistributionNumber(double mean, double sigma);
		bool BernoulliTrial(double prob);

	private:
		boost::random::mt19937 _rng;
	};

	NormalRand rnd;

	int NormalRand::number(int from, int to)
	{
#if BOOST_VERSION >= 104700
		boost::random::uniform_int_distribution<> dist(from, to);
		return dist(_rng);
#else
		boost::uniform_int<> dist(from, to);
		boost::variate_generator<boost::mt19937&, boost::uniform_int<> >  dice(rng, dist);
		return dice();
#endif
	}

	//������� ���������� ��������� ����� � ���������� ��������������
	//� ����������� mean - ����������� � sigma - ���������
	double NormalRand::NormalDistributionNumber(double mean, double sigma)
	{
#if BOOST_VERSION >= 104700
		//Static is necessary, see http://www.bnikolic.co.uk/blog/cpp-boost-uniform01.html
		static boost::uniform_01<boost::mt19937> _un_rng(_rng);
		boost::random::normal_distribution<double> NormalDistribution(mean, sigma);
		return NormalDistribution(_un_rng);
#else
		// ���������� �� ������ ��� ��� �����������, � ��� ����� ����� ��������...
		boost::normal_distribution<double> NormDist(mean, sigma);
		boost::variate_generator< mt19937&, normal_distribution<double> > NormalDistribution(rng, NormDist);
		return NormalDistribution();
#endif
	}

	bool NormalRand::BernoulliTrial(double p)
	{
		boost::random::binomial_distribution<> dist(p);
		return dist(_rng);
	}
} // namespace Random

// ������� ����� ������, ����� ������� ��� ��� ������� � ��������� ���� ��� ��������

// * ��������� ���������� ����� � ��������� �� from �� to.
int number(int from, int to)
{
	if (from > to)
	{
		int tmp = from;
		from = to;
		to = tmp;
	}

	return Random::rnd.number(from, to);
}

// * ������ ������� �������.
int dice(int number, int size)
{
	int sum = 0;

	if (size <= 0 || number <= 0)
		return (0);

	while (number-- > 0)
		sum += Random::rnd.number(1, size);

	return sum;
}

bool bernoulli_trial(double p)
{
	return Random::rnd.BernoulliTrial(p);
}

/**
* ��������� ������ ����� � �������� ��������� � ���������� ��������������
  ������ ������, ��� ��� ���������, �� ����� �� ���� �������� �������� ��� ����.
  ����� ���� ��������� ����������������. ��� ������� ����� ��������� ���������
  ��������, ������� �� mean, ����� ����� ������������. ��� ������� ������� �����
  � meam, ������� � ���� �������, � ������� ������� �������� ������� ������� � ��� ����.
  ������ �� ������ � ������� ���, ��� ��� ������������ ���������.
*/
int GaussIntNumber(double mean, double sigma, int min_val, int max_val)
{
	double dresult = 0.0;
	int iresult = 0;

	// �������� ��������� ����� � �������� ������������ � ��������������
	dresult = Random::rnd.NormalDistributionNumber(mean, sigma);
	//(dresult < mean) ? (iresult = ceil(dresult)) : iresult = floor(dresult);
	//���������
	iresult = ((dresult < mean) ? ceil(dresult) : floor(dresult));
	//���������� � ���������� �� ���������
	return MAX(MIN(iresult, max_val), min_val);
}


// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
