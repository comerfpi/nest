/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   testNEST.cpp
 * Author: brodsky3
 *
 * Created on August 1, 2017, 1:03 PM
 */

#include <NEST.hh>
#include <TestSpectra.hh>
#include <float.h>
#include <iostream>

using namespace std;
using namespace NEST;

/*
 * 
 */

NEST::NESTcalc n; std::default_random_engine generator;
double nCr ( double n, double r );

int main ( int argc, char** argv ) {
  
  
//  double xMin, xMax, yMax, FuncValue;
  
  if (argc < 7)
  {
    cout << "This program takes 6 (or 7) inputs." << endl << endl;
    cout << "numEvts type_interaction E_min[keV] E_max[keV] density[g/cm^3] field_drift[V/cm] {optional:seed}" << endl;
    cout << "for 8B or WIMPs, numEvts is kg-days of exposure" << endl << endl;
    cout << "exposure[kg-days] {WIMP} m[GeV] x-sect[cm^2] density[g/cm^3] field_drift[V/cm] {optional:seed}" << endl;
    return 0;
  }
  unsigned long int numEvts = atoi(argv[1]);

  string type = argv[2];
  INTERACTION_TYPE type_num;
  WIMP_spectrum_prep wimp_spectrum_prep; //used only in WIMP case
  if (type == "NR") type_num = NR;
  else if (type == "WIMP")
  {
    type_num = WIMP;
    wimp_spectrum_prep= WIMP_prep_spectrum(atof(argv[3]));
    numEvts = n.poisson_draw(wimp_spectrum_prep.integral * atof(argv[1]) * atof(argv[4]) / 1e-36);
  } else if (type == "B8")
  {
    type_num = B8;
    numEvts = n.poisson_draw(0.0026 * atof(argv[1]));
  } else if (type == "DD") type_num = DD;
  else if (type == "AmBe")type_num = AmBe;
  else if (type == "Cf") type_num = Cf;
  else if (type == "ion") type_num = ion;
  else if (type == "gamma")type_num = gammaRay;
  else if (type == "Kr83m")type_num=Kr83m;
  else if (type == "CH3T")type_num = CH3T;
  else type_num = beta;

  double eMin = atof(argv[3]);
  double eMax = atof(argv[4]);
  double rho = atof(argv[5]);
  double field = atof(argv[6]);

  if ( type_num == Kr83m && eMin == 9.4 && eMax == 9.4 )
    fprintf(stdout, "t [ns]\t\tE [keV]\t\tNph\tNe-\tS1_raw [PE]\tS1_Zcorr\tS1c_spike\tNe-X\tS2_rawArea\tS2_Zcorr [phd]\n");
  else
    fprintf(stdout, "E [keV]\t\tNph\tNe-\tS1_raw [PE]\tS1_Zcorr\tS1c_spike\tNe-X\tS2_rawArea\tS2_Zcorr [phd]\n");

  if (argc >= 8) n.SetRandomSeed(atoi(argv[7]));
    
    double keV = -999;
    for (unsigned long int j = 0; j < numEvts; j++) {
      if (eMin == eMax) {
	keV = eMin;
      } else {
	switch (type_num) {
	case CH3T:
	  keV = CH3T_spectrum(eMin, eMax, n);
	  break;
	case B8: //normalize this to ~3500 / 10-ton / year, for E-threshold of 0.5 keVnr, OR 180 evts/t/yr/keV at 1 keV
	  keV = B8_spectrum(eMin, eMax, n);
	  break;
	case AmBe: //for ZEPLIN-III FSR from HA (Pal '98)
	  keV = AmBe_spectrum(eMin, eMax, n);
	  break;
	case Cf:
	  keV = Cf_spectrum(eMin, eMax, n);
	  break;
	case DD:
	  keV = DD_spectrum(eMin, eMax, n);
	  break;
	case WIMP:
          {          
          keV = WIMP_spectrum(wimp_spectrum_prep, atof(argv[3]),n);
          }
          break;
	default:
	  keV = eMin + (eMax - eMin) * n.rand_uniform();
	  break;
	}
      }
      
      if ( type_num != WIMP ) {
	if (keV > eMax) keV = eMax;
	if (keV < eMin) keV = eMin;
      }
      
      NEST::YieldResult yields = n.GetYields ( type_num, keV, rho, field );
      vector<double> scint = n.GetS1(int(floor(yields.PhotonYield+0.5)));
      printf("%.6f\t%.6f\t%.6f\t", keV, yields.PhotonYield, yields.ElectronYield);
      printf("%.6f\t%.6f\t%.6f\t", scint[2], scint[5], scint[7]);
      scint = n.GetS2(int(floor(yields.ElectronYield+0.5)));
      printf("%i\t%.6f\t%.6f\n", (int)scint[0], scint[4], scint[7]);
    }
    
    return 1;
    
}

vector<double> NESTcalc::GetS1 ( int Nph ) {
  
  vector<double> scintillation(8);
  int coinLevel = 3,numPMTs = 100;
  double g1 = 0.10, sPEres = 0.5, P_dphe = 0.2, sPEeff = 0.92, sPEthr = 0.25;
  double posDep = 0.9 + n.rand_uniform()*(1.1-0.9);
  
  int nHits=n.BinomFluct(Nph,g1*posDep), Nphe = 0;
  double pulseArea = 0., spike = 0., prob;
  if ( sPEthr > 0. ) {
    for ( int i = 0; i < nHits; i++ ) {
      double phe1 = n.rand_gauss(1.,sPEres); Nphe++;
      prob = n.rand_uniform();
      if ( prob > sPEeff ) { phe1 = 0.; } //add an else with Nphe++ if not doing mc truth
      double phe2 = 0.;
      if ( n.rand_uniform() < P_dphe ) {
	phe2 = n.rand_gauss(1.,sPEres); Nphe++;
	if ( n.rand_uniform() > sPEeff && prob > sPEeff ) { phe2 = 0.; } //add an else with Nphe++ if not doing mc truth
      }
      if ( (phe1+phe2) > sPEthr ) { spike++; pulseArea += phe1 + phe2; }
    }
  }
  else {
    Nphe = nHits + n.BinomFluct(nHits,P_dphe);
    pulseArea = n.rand_gauss(n.BinomFluct(Nphe,1.-(1.-sPEeff)/(1.+P_dphe)),sPEres*sqrt(Nphe));
    spike = (double)nHits;
  }
  if ( pulseArea < 0. ) pulseArea = 0.;
  double pulseAreaC= pulseArea / posDep;
  double Nphd = pulseArea / (1.+P_dphe);
  double NphdC= pulseAreaC/ (1.+P_dphe);
  double spikeC = spike / posDep;
  
  scintillation[0] = nHits; scintillation[1] = Nphe;
  scintillation[2] = pulseArea; scintillation[3] = pulseAreaC;
  scintillation[4] = Nphd; scintillation[5] = NphdC;
  scintillation[6] = spike; scintillation[7] = spikeC;
  
  if ( spike >= coinLevel ) { double numer, denom;
    for ( int i = spike; i > 0; i-- ) {
      denom += nCr ( numPMTs, i );
      if ( i >= coinLevel ) numer += nCr ( numPMTs, i );
    }
    prob = numer / denom;
  } else prob = 0.; if ( spike > 10 ) prob = 1.;
  
  if ( n.rand_uniform() < prob )
    { ; }
  else {
    //scintillation[0] *= -1.;
    //scintillation[1] *= -1.;
    scintillation[2] *= -1.;
    scintillation[3] *= -1.;
    //scintillation[4] *= -1.;
    //scintillation[5] *= -1.;
    scintillation[6] *= -1.;
    scintillation[7] *= -1.;
  }
  
  return scintillation;
  
}

vector<double> NESTcalc::GetS2 ( int Ne ) {
  
  vector<double> ionization(8);
  double alpha = 0.137, beta = 177., gamma = 45.7, eLife_us = 500., P_dphe = 0.2, sPEres = 0.5, Fano = 3., S2botTotRatio = 0.4;
  double driftTime = 0.0 + n.rand_uniform()*(500.-0.0);
  double g1_gas = 0.10, gasGap_cm = 0.5,p_bar = 1.5,E_gas=10.,epsilon=1.85/1.00126;
  
  double E_liq = E_gas / epsilon; //kV per cm
  double ExtEff = -0.03754*pow(E_liq,2.)+0.52660*E_liq-0.84645; // arXiv:1710.11032
  if ( ExtEff > 1. ) ExtEff = 1.;
  if ( ExtEff < 0. ) ExtEff = 0.;
  int Nee = n.BinomFluct(Ne,ExtEff*exp(-driftTime/eLife_us));
  
  double elYield = Nee*
    (alpha*E_gas*1000.-beta*p_bar-gamma)*
    gasGap_cm; // arXiv:1207.2292
  int Nph = int(floor(rand_gauss(elYield,sqrt(Fano*elYield))+0.5));
  int nHits = n.BinomFluct(Nph,g1_gas);
  int Nphe = nHits + n.BinomFluct(nHits,P_dphe);
  double pulseArea=n.rand_gauss(Nphe,sPEres*sqrt(Nphe));
  double pulseAreaC= pulseArea/exp(-driftTime/eLife_us);
  double Nphd = pulseArea / (1.+P_dphe);
  double NphdC= pulseAreaC/ (1.+P_dphe);
  
  double S2b = n.rand_gauss(S2botTotRatio*pulseArea,sqrt(S2botTotRatio*pulseArea*(1.-S2botTotRatio)));
  double S2bc= S2b / exp(-driftTime/eLife_us);
  
  ionization[0] = Nee; ionization[1] = Nph;
  ionization[2] = nHits; ionization[3] = Nphe;
  ionization[4] = pulseArea; ionization[5] = pulseAreaC;
  ionization[6] = Nphd; ionization[7] = NphdC;
  
  return ionization;
  
}



long double Factorial ( double x ) {
  
  return tgammal ( x + 1. );
  
}

double nCr ( double n, double r ) {
  
  return Factorial(n) /
    ( Factorial(r) * Factorial(n-r) );
  
}
