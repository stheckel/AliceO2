// $Id: AliHLTTPCGMTrackParam.cxx 41769 2010-06-16 13:58:00Z sgorbuno $
// **************************************************************************
// This file is property of and copyright by the ALICE HLT Project          *
// ALICE Experiment at CERN, All rights reserved.                           *
//                                                                          *
// Primary Authors: Sergey Gorbunov <sergey.gorbunov@kip.uni-heidelberg.de> *
//                  for The ALICE HLT Project.                              *
//                                                                          *
// Permission to use, copy, modify and distribute this software and its     *
// documentation strictly for non-commercial purposes is hereby granted     *
// without fee, provided that the above copyright notice appears in all     *
// copies and that both the copyright notice and this permission notice     *
// appear in the supporting documentation. The authors make no claims       *
// about the suitability of this software for any purpose. It is            *
// provided "as is" without express or implied warranty.                    *
//                                                                          *
//***************************************************************************

#include "AliHLTTPCGMTrackParam.h"
#include "AliHLTTPCCAMath.h"
#include "AliHLTTPCGMTrackLinearisation.h"
#include "AliHLTTPCGMBorderTrack.h"
#include "AliHLTTPCGMMergedTrack.h"
#include "Riostream.h"
#ifndef HLTCA_STANDALONE
#include "AliExternalTrackParam.h"
#endif
#include "AliHLTTPCCAParam.h"
#include <cmath>

GPUd() void AliHLTTPCGMTrackParam::Fit
(
 float* PolinomialFieldBz,
 float x[], float y[], float z[], int rowType[], float alpha[], const AliHLTTPCCAParam &param,
 int &N,
 float &Alpha,
 bool UseMeanPt,
 float maxSinPhi
 ){
  int nWays = param.GetNWays();
  int maxN = N;
  for (int iWay = 0;iWay < nWays;iWay++)
  {
    AliHLTTPCGMTrackLinearisation t0(*this);
    float trDzDs2 = t0.DzDs()*t0.DzDs();
 
    AliHLTTPCGMTrackFitParam par;

    int first = 1;
    N = 0;
    int iihit;
    for( iihit=0; iihit<maxN; iihit++)
    {
      const int ihit = (iWay & 1) ? (maxN - iihit - 1) : iihit;
      if (rowType[ihit] < 0) continue;
      float dL = 0;
      float ex1i = 0;
      if (PropagateTrack(PolinomialFieldBz, x[ihit], y[ihit], z[ihit], alpha[ihit], rowType[ihit], param, N, Alpha, maxSinPhi, UseMeanPt, first, par, t0, dL, ex1i, trDzDs2))
      {
        if (first)
        {
            for (int i = 0;i < 15;i++) fC[i] = 0;
            fChi2 = 0.;
            fNDF = 1;
        }
        break;
      }
      if (first == 0)
      {
        bool rejectThisRound = nWays == 1 || iWay == 1;
        int retVal = UpdateTrack( y[ihit], z[ihit], rowType[ihit], param, N, maxSinPhi, par, dL, ex1i, trDzDs2, rejectThisRound);
        if (retVal == 0) {}
        else if (retVal == 2)
        {
          if (rejectThisRound && !(param.HighQPtForward() < fabs(QPt()))) rowType[ihit] = -(rowType[ihit] + 1);
        }
        else break;
      }
      first = 0;
    }
    maxN = iihit;
  }
}

GPUd() int AliHLTTPCGMTrackParam::PropagateTrack(float* PolinomialFieldBz, float posX, float posY, float posZ, float posAlpha, int rowType, const AliHLTTPCCAParam &param, int& N, float& Alpha, float maxSinPhi, bool UseMeanPt, int first, AliHLTTPCGMTrackFitParam& par, AliHLTTPCGMTrackLinearisation& t0, float& dL, float& ex1i, float trDzDs2)
{
    float sliceAlpha = posAlpha;
    
    if ( fabs( sliceAlpha - Alpha ) > 1.e-4 ) {
      if( !Rotate(  sliceAlpha - Alpha, t0, .999 ) ) return 1;
      Alpha = sliceAlpha;
    }

    float bz = GetBz(posX, posY, param.GetContinuousTracking() ? (posZ > 0 ? 125. : -125.) : posZ, PolinomialFieldBz);
        
    { // transport block
      
      bz = -bz;

      float ex = t0.CosPhi();
      
      float ey = t0.SinPhi();
      float k  = t0.QPt()*bz;
      float dx = posX - X();
      float kdx = k*dx;
      float ey1 = kdx + ey;
      
      if( fabs( ey1 ) >= maxSinPhi ) return 1;

      float ss = ey + ey1;   
      float ex1 = sqrt(1 - ey1*ey1);
      
      float dxBz = dx * bz;
    
      float cc = ex + ex1;  
      float dxcci = dx * Reciprocal(cc);
      float kdx205 = kdx*kdx*0.5f;
      
      float dy = dxcci * ss;      
      float norm2 = float(1.f) + ey*ey1 + ex*ex1;
      float dl = dxcci * sqrt( norm2 + norm2 );

      float dS;    
      { 
        float dSin = float(0.5f)*k*dl;
        float a = dSin*dSin;
        const float k2 = 1.f/6.f;
        const float k4 = 3.f/40.f;
        //const float k6 = 5.f/112.f;
        dS = dl + dl*a*(k2 + a*(k4 ));//+ k6*a) );
      }
      
      ex1i = Reciprocal(ex1);
      float dz = dS * t0.DzDs();  
      
      dL = -dS * t0.DlDs();
      
      float hh = dxcci*ex1i*(2.f+kdx205); 
      float h2 = hh * t0.SecPhi();
      float h4 = bz*dxcci*hh;

      float d2 = fP[2] - t0.SinPhi();
      float d3 = fP[3] - t0.DzDs();
      float d4 = fP[4] - t0.QPt();
      
      
      fX+=dx;
      fP[0]+= dy     + h2 * d2           +   h4 * d4;
      fP[1]+= dz               + dS * d3;
      fP[2] = ey1 +     d2           + dxBz * d4;    
      
      t0.CosPhi() = ex1;
      t0.SecPhi() = ex1i;
      t0.SinPhi() = ey1;      


      if ( first ) {
        float err2Y, err2Z;
        {
          const float *cy = param.GetParamS0Par(0,rowType);
          const float *cz = param.GetParamS0Par(1,rowType);

          float secPhi2 = ex1i*ex1i;
          const float kZLength = 250.f - 0.275f;
          float zz = param.GetContinuousTracking() ? 125. : fabs( kZLength - fabs(fP[1]) );
          float zz2 = zz*zz;
          float angleY2 = secPhi2 - 1.f; 
          const float trDzDs2 = t0.DzDs()*t0.DzDs();
          float angleZ2 = trDzDs2 * secPhi2 ;

          float cy0 = cy[0] + cy[1]*zz + cy[3]*zz2;
          float cy1 = cy[2] + cy[5]*zz;
          float cy2 = cy[4];
          float cz0 = cz[0] + cz[1]*zz + cz[3]*zz2;
          float cz1 = cz[2] + cz[5]*zz;
          float cz2 = cz[4];

          err2Y = fabs( cy0 + angleY2 * ( cy1 + angleY2*cy2 ) );
          err2Z = fabs( cz0 + angleZ2 * ( cz1 + angleZ2*cz2 ) );      
        }
        
        fP[0] = posY;
        fP[1] = posZ;
        SetCov( 0, err2Y );
        SetCov( 1,  0 );
        SetCov( 2, err2Z);
        SetCov( 3,  0 );
        SetCov( 4,  0 );
        SetCov( 5,  1 );
        SetCov( 6,  0 );
        SetCov( 7,  0 );
        SetCov( 8,  0 );
        SetCov( 9,  1 );
        SetCov( 10,  0 );
        SetCov( 11,  0 );
        SetCov( 12,  0 );
        SetCov( 13,  0 );
        SetCov( 14,  10 );
        SetChi2( 0 );
        SetNDF( -3 );
        const float kRho = 1.025e-3;//0.9e-3;
        const float kRadLen = 29.532;//28.94;
        const float kRhoOverRadLen = kRho / kRadLen;
        CalculateFitParameters( par, kRhoOverRadLen, kRho, UseMeanPt );
        N+=1;
        return 0;
      }

      float c20 = fC[3];
      float c21 = fC[4];
      float c22 = fC[5];
      float c30 = fC[6];
      float c31 = fC[7];
      float c32 = fC[8];
      float c33 = fC[9];
      float c40 = fC[10];
      float c41 = fC[11];
      float c42 = fC[12];
      float c43 = fC[13];
      float c44 = fC[14];
      
      float c20ph4c42 =  c20 + h4*c42;
      float h2c22 = h2*c22;
      float h4c44 = h4*c44;
      
      float n6 = c30 + h2*c32 + h4*c43;
      float n7 = c31 + dS*c33;
      float n10 = c40 + h2*c42 + h4c44;
      float n11 = c41 + dS*c43;
      float n12 = c42 + dxBz*c44;
      
      fC[8] = c32 + dxBz * c43;
      
      fC[0]+= h2*h2c22 + h4*h4c44 + float(2.f)*( h2*c20ph4c42  + h4*c40 );
      
      fC[1]+= h2*c21 + h4*c41 + dS*n6;
      fC[6] = n6;
      
      fC[2]+= dS*(c31 + n7);
      fC[7] = n7; 
      
      fC[3] = c20ph4c42 + h2c22  + dxBz*n10;
      fC[10] = n10;
      
      fC[4] = c21 + dS*c32 + dxBz*n11;
      fC[11] = n11;
      
      fC[5] = c22 + dxBz*( c42 + n12 );
      fC[12] = n12;
      
    } // end transport block 
    
    return 0;
}

GPUd() int AliHLTTPCGMTrackParam::UpdateTrack( float posY, float posZ, int rowType, const AliHLTTPCCAParam &param, int& N, float maxSinPhi, AliHLTTPCGMTrackFitParam& par, float& dL, float& ex1i, float trDzDs2, bool rejectChi2)
{
	if (fabs(posY - fP[0]) > 3 || fabs(posZ - fP[1]) > 3) return 2;
	
    float &fC22 = fC[5];
    float &fC33 = fC[9];
    float &fC40 = fC[10];
    float &fC41 = fC[11];
    float &fC42 = fC[12];
    float &fC43 = fC[13];
    float &fC44 = fC[14];
    
    float 
      c00 = fC[ 0],
      c11 = fC[ 2],
      c20 = fC[ 3],
      c31 = fC[ 7];

    //Copy computation of err2? from first propagation (above) for update
    float err2Y, err2Z;
    {
      const float *cy = param.GetParamS0Par(0,rowType);
      const float *cz = param.GetParamS0Par(1,rowType);

      float secPhi2 = ex1i*ex1i;
      const float kZLength = 250.f - 0.275f;
      float zz = param.GetContinuousTracking() ? 125. : fabs( kZLength - fabs(fP[1]) );
      float zz2 = zz*zz;
      float angleY2 = secPhi2 - 1.f; 
      float angleZ2 = trDzDs2 * secPhi2 ;

      float cy0 = cy[0] + cy[1]*zz + cy[3]*zz2;
      float cy1 = cy[2] + cy[5]*zz;
      float cy2 = cy[4];
      float cz0 = cz[0] + cz[1]*zz + cz[3]*zz2;
      float cz1 = cz[2] + cz[5]*zz;
      float cz2 = cz[4];

      err2Y = fabs( cy0 + angleY2 * ( cy1 + angleY2*cy2 ) );
      err2Z = fabs( cz0 + angleZ2 * ( cz1 + angleZ2*cz2 ) );      
    }


    // MS block  
    
    float dLmask = 0.f;
    bool maskMS = ( fabs( dL ) < par.fDLMax );

    
    // Filter block
    
    float mS0 = Reciprocal(err2Y + c00);
    
    // MS block
    Assign( dLmask, maskMS, dL );
    
    // Filter block
    
    float  z0 = posY - fP[0];
    float  z1 = posZ - fP[1];
    float mS2 = Reciprocal(err2Z + c11);
    
    //printf("hits %d chi2 %f, new %f %f (dy %f dz %f)\n", N, fChi2, mS0 * z0 * z0, mS2 * z1 * z1, z0, z1);
    float tmpCut = param.HighQPtForward() < fP[4] ? 5 : 5;
    if (rejectChi2 && (mS0*z0*z0 > tmpCut || mS2*z1*z1 > tmpCut)) return 2;
    fChi2  += mS0*z0*z0;
    fChi2  +=  mS2*z1*z1;
    if (fChi2 / ((fNDF+5)/2 + 1) > 5) return 1;
    if( fabs( fP[2] + z0*c20*mS0  ) > maxSinPhi ) return 1;
    
    // MS block
    
    float dLabs = fabs( dLmask); 
    float corr = float(1.f) - par.fEP2* dLmask ;
    
    fC40 *= corr;
    fC41 *= corr;
    fC42 *= corr;
    fC43 *= corr;
    fC44  = fC44*corr*corr + dLabs*par.fSigmadE2;
    
    fC22 += dLabs * par.fK22 * (float(1.f)-fP[2]*fP[2]);
    fC33 += dLabs * par.fK33;
    fC43 += dLabs * par.fK43;
        

    // Filter block
  
    float c40 = fC40;
    
    // K = CHtS
    
    float k00, k11, k20, k31, k40;
    
    k00 = c00 * mS0;
    k20 = c20 * mS0;
    k40 = c40 * mS0;
  
    
    k11 = c11 * mS2;
    k31 = c31 * mS2;
    
    fNDF  += 2;
    N+=1;
    
    fP[0] += k00 * z0;
    fP[1] += k11 * z1;
    fP[2] += k20 * z0;
    fP[3] += k31 * z1;
    fP[4]*= corr;
    fP[4] += k40 * z0;
    
    fC[ 0] -= k00 * c00 ;
    fC[ 2] -= k11 * c11;
    fC[ 3] -= k20 * c00 ;
    fC[ 5] -= k20 * c20 ;
    fC[ 7] -= k31 * c11;
    fC[ 9] -= k31 * c31;
    fC[10] -= k00 * c40 ;
    fC[12] -= k40 * c20 ;
    fC[14] -= k40 * c40 ;
    
    return 0;
}

GPUd() bool AliHLTTPCGMTrackParam::CheckNumericalQuality() const
{
  //* Check that the track parameters and covariance matrix are reasonable
  bool ok = AliHLTTPCCAMath::Finite(fX) && AliHLTTPCCAMath::Finite( fChi2 ) && AliHLTTPCCAMath::Finite( fNDF );

  const float *c = fC;
  for ( int i = 0; i < 15; i++ ) ok = ok && AliHLTTPCCAMath::Finite( c[i] );
  for ( int i = 0; i < 5; i++ ) ok = ok && AliHLTTPCCAMath::Finite( fP[i] );
  
  if ( c[0] <= 0 || c[2] <= 0 || c[5] <= 0 || c[9] <= 0 || c[14] <= 0 ) ok = 0;
  if ( c[0] > 5. || c[2] > 5. || c[5] > 2. || c[9] > 2. 
       //|| ( CAMath::Abs( QPt() ) > 1.e-2 && c[14] > 2. ) 
       ) ok = 0;

  if ( fabs( fP[2] ) > .999 ) ok = 0;
  if( ok ){
    ok = ok 
      && ( c[1]*c[1]<=c[2]*c[0] )
      && ( c[3]*c[3]<=c[5]*c[0] )
      && ( c[4]*c[4]<=c[5]*c[2] )
      && ( c[6]*c[6]<=c[9]*c[0] )
      && ( c[7]*c[7]<=c[9]*c[2] )
      && ( c[8]*c[8]<=c[9]*c[5] )
      && ( c[10]*c[10]<=c[14]*c[0] )
      && ( c[11]*c[11]<=c[14]*c[2] )
      && ( c[12]*c[12]<=c[14]*c[5] )
      && ( c[13]*c[13]<=c[14]*c[9] );      
  }
  return ok;
}

//*
//*  Multiple scattering and energy losses
//*

GPUd() float AliHLTTPCGMTrackParam::ApproximateBetheBloch( float beta2 )
{
  //------------------------------------------------------------------
  // This is an approximation of the Bethe-Bloch formula with
  // the density effect taken into account at beta*gamma > 3.5
  // (the approximation is reasonable only for solid materials)
  //------------------------------------------------------------------

  const float log0 = log( float(5940.f));
  const float log1 = log( float(3.5f*5940.f) );

  bool bad = (beta2 >= .999f)||( beta2 < 1.e-8f );

  Assign( beta2, bad, 0.5f);

  float a = beta2 / ( 1.f - beta2 ); 
  float b = 0.5*log(a);
  float d =  0.153e-3 / beta2;
  float c = b - beta2;

  float ret = d*(log0 + b + c );
  float case1 = d*(log1 + c );
  
  Assign( ret, ( a > 3.5*3.5  ), case1);
  Assign( ret,  bad, 0. ); 

  return ret;
}

 GPUd() void AliHLTTPCGMTrackParam::CalculateFitParameters( AliHLTTPCGMTrackFitParam &par, float RhoOverRadLen,  float Rho, bool NoField, float mass )
{
  //*!

  float qpt = fP[4];
  if( NoField ) qpt = 1./0.35;

  float p2 = ( 1. + fP[3] * fP[3] );
  float k2 = qpt * qpt;
  Assign( k2, (  k2 < 1.e-4f ), 1.e-4f );

  float mass2 = mass * mass;
  float beta2 = p2 / ( p2 + mass2 * k2 );
  
  float pp2 = p2 / k2; // impuls 2

  //par.fBethe = BetheBlochGas( pp2/mass2);
  par.fBetheRho = ApproximateBetheBloch( pp2 / mass2 )*Rho;
  par.fE = sqrt( pp2 + mass2 );
  par.fTheta2 = ( 14.1*14.1/1.e6 ) / ( beta2 * pp2 )*RhoOverRadLen;
  par.fEP2 = par.fE / pp2;

  // Approximate energy loss fluctuation (M.Ivanov)

  const float knst = 0.07; // To be tuned.
  par.fSigmadE2 = knst * par.fEP2 * qpt;
  par.fSigmadE2 = par.fSigmadE2 * par.fSigmadE2;
  
  float k22 = 1. + fP[3] * fP[3];
  par.fK22 = par.fTheta2*k22;
  par.fK33 = par.fK22 * k22;
  par.fK43 = 0.;
  par.fK44 =  par.fTheta2*fP[3] * fP[3] * k2;
  
  float br=1.e-8f;
  Assign( br, ( par.fBetheRho>1.e-8f ), par.fBetheRho );
  par.fDLMax = 0.3*par.fE * Reciprocal( br );

  par.fEP2*=par.fBetheRho;
  par.fSigmadE2 = par.fSigmadE2*par.fBetheRho+par.fK44;  
}




//*
//* Rotation
//*


GPUd() bool AliHLTTPCGMTrackParam::Rotate( float alpha, AliHLTTPCGMTrackLinearisation &t0, float maxSinPhi )
{
  //* Rotate the coordinate system in XY on the angle alpha

  float cA = CAMath::Cos( alpha );
  float sA = CAMath::Sin( alpha );
  float x0 = X(), y0 = Y(), sP = t0.SinPhi(), cP = t0.CosPhi();
  float cosPhi = cP * cA + sP * sA;
  float sinPhi = -cP * sA + sP * cA;

  if ( CAMath::Abs( sinPhi ) > maxSinPhi || CAMath::Abs( cosPhi ) < 1.e-2 || CAMath::Abs( cP ) < 1.e-2  ) return 0;

  //float J[5][5] = { { j0, 0, 0,  0,  0 }, // Y
  //                    {  0, 1, 0,  0,  0 }, // Z
  //                    {  0, 0, j2, 0,  0 }, // SinPhi
  //                  {  0, 0, 0,  1,  0 }, // DzDs
  //                  {  0, 0, 0,  0,  1 } }; // Kappa

  float j0 = cP / cosPhi;
  float j2 = cosPhi / cP;
  float d[2] = {Y() - y0, SinPhi() - sP};

  X() = ( x0*cA +  y0*sA );
  Y() = ( -x0*sA +  y0*cA + j0*d[0] );
  t0.CosPhi() = fabs( cosPhi );
  t0.SecPhi() = ( 1./t0.CosPhi() );
  t0.SinPhi() = ( sinPhi );

  SinPhi() = ( sinPhi + j2*d[1] );

  fC[0] *= j0 * j0;
  fC[1] *= j0;
  fC[3] *= j0;
  fC[6] *= j0;
  fC[10] *= j0;

  fC[3] *= j2;
  fC[4] *= j2;
  fC[5] *= j2 * j2;
  fC[8] *= j2;
  fC[12] *= j2;
  if( cosPhi <0 ){ // change direction
    t0.SinPhi() = -sinPhi;
    t0.DzDs() = -t0.DzDs();
    t0.DlDs() = -t0.DlDs();
    t0.QPt() = -t0.QPt();
    SinPhi() = -SinPhi();
    DzDs() = -DzDs();
    QPt() = -QPt();
    fC[3] = -fC[3];
    fC[4] = -fC[4];
    fC[6] = -fC[6];
    fC[7] = -fC[7];
    fC[10] = -fC[10];
    fC[11] = -fC[11];
  }

  return 1;
}

#if !defined(HLTCA_STANDALONE) & !defined(HLTCA_GPUCODE)
bool AliHLTTPCGMTrackParam::GetExtParam( AliExternalTrackParam &T, double alpha ) const
{
  //* Convert from AliHLTTPCGMTrackParam to AliExternalTrackParam parameterisation,
  //* the angle alpha is the global angle of the local X axis

  bool ok = CheckNumericalQuality();

  double par[5], cov[15];
  for ( int i = 0; i < 5; i++ ) par[i] = fP[i];
  for ( int i = 0; i < 15; i++ ) cov[i] = fC[i];

  if ( par[2] > .99 ) par[2] = .99;
  if ( par[2] < -.99 ) par[2] = -.99;

  if ( fabs( par[4] ) < 1.e-5 ) par[4] = 1.e-5; // some other software will crash if q/Pt==0
  if ( fabs( par[4] ) > 1./0.08 ) ok = 0; // some other software will crash if q/Pt is too big

  T.Set( (double) fX, alpha, par, cov );
  return ok;
}


 
void AliHLTTPCGMTrackParam::SetExtParam( const AliExternalTrackParam &T )
{
  //* Convert from AliExternalTrackParam parameterisation

  for ( int i = 0; i < 5; i++ ) fP[i] = T.GetParameter()[i];
  for ( int i = 0; i < 15; i++ ) fC[i] = T.GetCovariance()[i];
  fX = T.GetX();
  if ( fP[2] > .999 ) fP[2] = .999;
  if ( fP[2] < -.999 ) fP[2] = -.999;
}
#endif

GPUd() void AliHLTTPCGMTrackParam::RefitTrack(AliHLTTPCGMMergedTrack &track, float* PolinomialFieldBz, float* x, float* y, float* z, int* rowType, float* alpha, const AliHLTTPCCAParam& param)
{
	if( !track.OK() ) return;    

	int nTrackHits = track.NClusters();
	   
	AliHLTTPCGMTrackParam t = track.Param();
	float Alpha = track.Alpha();  
	int nTrackHitsOld = nTrackHits;
	float ptOld = t.QPt();
	t.Fit( PolinomialFieldBz,
	   x+track.FirstClusterRef(),
	   y+track.FirstClusterRef(),
	   z+track.FirstClusterRef(),
	   rowType+track.FirstClusterRef(),
	   alpha+track.FirstClusterRef(),
	   param, nTrackHits, Alpha );      
	
	if ( fabs( t.QPt() ) < 1.e-4 ) t.QPt() = 1.e-4 ;
	bool okhits = nTrackHits >= TRACKLET_SELECTOR_MIN_HITS(track.Param().QPt());
	bool okqual = t.CheckNumericalQuality();
	bool okphi = fabs( t.SinPhi() ) <= .999;
			
	bool ok = okhits && okqual && okphi;

	//printf("Track %d OUTPUT hits %d -> %d, QPt %f -> %f, ok %d (%d %d %d) chi2 %f chi2ndf %f\n", blanum,  nTrackHitsOld, nTrackHits, ptOld, t.QPt(), (int) ok, (int) okhits, (int) okqual, (int) okphi, t.Chi2(), t.Chi2() / max(1,nTrackHits);
	if (param.HighQPtForward() < fabs(track.Param().QPt()))
	{
		ok = 1;
		nTrackHits = nTrackHitsOld;
		for (int k = 0;k < nTrackHits;k++) if (rowType[k] < 0) rowType[k] = -rowType[k] - 1;
	}
	track.SetOK(ok);
	if (!ok) return;

	if( 1 ){//SG!!!
	  track.SetNClusters( nTrackHits );
	  track.Param() = t;
	  track.Alpha() = Alpha;
	}

	{
	  int ind = track.FirstClusterRef();
	  float alphaa = alpha[ind];
	  float xx = x[ind];
	  float yy = y[ind];
	  float zz = z[ind];
	  float sinA = AliHLTTPCCAMath::Sin( alphaa - track.Alpha());
	  float cosA = AliHLTTPCCAMath::Cos( alphaa - track.Alpha());
	  track.SetLastX( xx*cosA - yy*sinA );
	  track.SetLastY( xx*sinA + yy*cosA );
	  track.SetLastZ( zz );
	}
}

#ifdef HLTCA_GPUCODE

GPUg() void RefitTracks(AliHLTTPCGMMergedTrack* tracks, int nTracks, float* PolinomialFieldBz, float* x, float* y, float* z, int* rowType, float* alpha, AliHLTTPCCAParam* param)
{
	for (int i = get_global_id(0);i < nTracks;i += get_global_size(0))
	{
		AliHLTTPCGMTrackParam::RefitTrack(tracks[i], PolinomialFieldBz, x, y, z, rowType, alpha, *param);
	}
}

#endif
