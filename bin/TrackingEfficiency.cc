////////////////////////////////////////////////////////////////////
//
// Dean Andrew Hidas <Dean.Andrew.Hidas@cern.ch>
//
// Created on: Mon Jul  4 19:20:41 CEST 2011
//
////////////////////////////////////////////////////////////////////


#include <iostream>
#include <string>

#include "PLTEvent.h"
#include "PLTU.h"
#include "TFile.h"



class HitCounter
{
  public:
    HitCounter () {
      NFiducial[0] = 0;
      NFiducial[1] = 0;
      NFiducial[2] = 0;
      NFiducialAndHit[0] = 0;
      NFiducialAndHit[1] = 0;
      NFiducialAndHit[2] = 0;
    }
    ~HitCounter () {}

    int NFiducial[3];
    int NFiducialAndHit[3];
};



// FUNCTION DEFINITIONS HERE
int TrackingEfficiency (std::string const, std::string const, std::string const);







// CODE BELOW
float transformSlopeY(float slopeX, float slopeY){
  //float p0 = -0.00051747;
  // float p1 = -0.000125617;
  // float p2 = -6.20074;
  // float p3 = 352.422;
  // float p4 = -75028.4;
  // float ret = slopeY - (p1*slopeY) - (p2*slopeY*slopeY) - (p3*slopeY*slopeY*slopeY) - (p4*slopeY*slopeY*slopeY*slopeY);

  //old params
  // float p2 = -11.8481;
  // float p4  = -3797.14;
 
  //new params
  float p2 = -11.905;
  float p4 = 3603.928;
  float ret;
  if (fabs(slopeY)<0.05)
    ret = slopeY - (p2*slopeX*slopeX) - (p4*slopeX*slopeX*slopeX*slopeX);
  else
    ret = slopeY;

  return ret;
}
TH1F* FidHistFrom2D (TH2F* hIN, TString const NewName, int const NBins, PLTPlane::FiducialRegion FidRegion)
{
  // This function returns a TH1F* and YOU are then the owner of
  // that memory.  please delete it when you are done!!!

  int const NBinsX = hIN->GetNbinsX();
  int const NBinsY = hIN->GetNbinsY();
  float const ZMin = 0;//hIN->GetMinimum();
  float const ZMax = hIN->GetMaximum() * (1.0 + 1.0 / (float) NBins);
  std::cout << hIN->GetMaximum() << "  " << ZMax << std::endl;
  int const MyNBins = NBins + 1;

  TString const hNAME = NewName == "" ? TString(hIN->GetName()) + "_1DZFid" : NewName;

  TH1F* h;
  h = new TH1F(hNAME, hNAME, MyNBins, ZMin, ZMax);
  h->SetXTitle("Number of Hits");
  h->SetYTitle("Number of Pixels");
  h->GetXaxis()->CenterTitle();
  h->GetYaxis()->CenterTitle();
  h->SetTitleOffset(1.4, "y");
  h->SetFillColor(40);

  for (int ix = 1; ix <= NBinsX; ++ix) {
    for (int iy = 1; iy <= NBinsY; ++iy) {
      int const px = hIN->GetXaxis()->GetBinLowEdge(ix);
      int const py = hIN->GetYaxis()->GetBinLowEdge(iy);
      if (PLTPlane::IsFiducial(FidRegion, px, py)) {
        if (hIN->GetBinContent(ix, iy) > ZMax) {
          h->Fill(ZMax - hIN->GetMaximum() / (float) NBins);
        } else {
          h->Fill( hIN->GetBinContent(ix, iy) );
        }
      }
    }
  }

  return h;
}

int TrackingEfficiency (std::string const DataFileName, std::string const GainCalFileName, std::string const AlignmentFileName)
{
  std::cout << "DataFileName:      " << DataFileName << std::endl;
  std::cout << "AlignmentFileName: " << AlignmentFileName << std::endl;

  // Set some basic style for plots
  PLTU::SetStyle();

  // Grab the plt event reader
  PLTEvent Event(DataFileName, GainCalFileName, AlignmentFileName, true);

  PLTPlane::FiducialRegion FidRegionHits  = PLTPlane::kFiducialRegion_All;
  //PLTPlane::FiducialRegion FidRegionTrack = PLTPlane::kFiducialRegion_m5_m5;
  PLTPlane::FiducialRegion FidRegionTrack = PLTPlane::kFiducialRegion_All;
  Event.SetPlaneFiducialRegion(FidRegionHits);
  Event.SetPlaneClustering(PLTPlane::kClustering_AllTouching,PLTPlane::kFiducialRegion_All);

  PLTAlignment Alignment;
  Alignment.ReadAlignmentFile(AlignmentFileName);

  // Map for tracking efficiency
  std::map<int, HitCounter> HC;
  std::map<int, TH2F*> hEffMapN;
  std::map<int, TH2F*> hEffMapD;
  std::map<int, TH1F*> hEffMapSlopeXN;
  std::map<int, TH1F*> hEffMapSlopeXD;
  std::map<int, TH1F*> hEffMapSlopeYN;
  std::map<int, TH1F*> hEffMapSlopeYD;
  std::map<int, TH1F*> hEffMapPulseHeightN;
  std::map<int, TH1F*> hEffMapPulseHeightD;
  std::map<int, TH1F*> hMapPulseHeights;
  std::map<int, TH1F*> hSlopeX;
  std::map<int, TH1F*> hSlopeY;
  std::map<int, TH2F*> hSlopeXvsSlopeY;
  std::map<int, TH2F*> hSlopeXvsSlopeY_Transformed;

  float const PixelDist = 5;

  // Loop over all events in file
  for (int ientry = 0; Event.GetNextEvent() >= 0; ++ientry) {
    if (ientry % 10000 == 0) {
      std::cout << "Processing entry: " << ientry << std::endl;
    }

    // Loop over all planes with hits in event
    for (size_t it = 0; it != Event.NTelescopes(); ++it) {
      PLTTelescope* Telescope = Event.Telescope(it);

      int    const Channel = Telescope->Channel();
      size_t const NPlanes = Telescope->NPlanes();

      // make them clean events
      if (Telescope->NHitPlanes() < 2 || Telescope->NHitPlanes() != Telescope->NClusters()) {
        continue;
      }


      // Make some hists for this telescope
      if (!hEffMapN.count(Channel * 10 + 0)) {

        TString Name = TString::Format("SlopeX_Ch%i", Channel);
        hSlopeX[Channel] = new TH1F(Name,Name,100,-0.1,0.1);
        Name = TString::Format("SlopeY_Ch%i", Channel);
        hSlopeY[Channel] = new TH1F(Name,Name,125,-0.1,0.1);
        Name = TString::Format("SlopeXvsSlopeY_Ch%i",Channel);
        hSlopeXvsSlopeY[Channel] = new TH2F(Name,Name,125,-0.1,0.1,100,-0.1,0.1);
        Name = TString::Format("SlopeXvsSlopeY_Transformed_Ch%i",Channel);
        hSlopeXvsSlopeY_Transformed[Channel] = new TH2F(Name,Name,125,-0.1,0.1,100,-0.1,0.1);


        // Make a numerator and demonitor hist for every roc for this channel
        for (int iroc = 0; iroc != 3; ++iroc) {
          Name = TString::Format("EffNumerator_Ch%i_ROC%i", Channel, iroc);
          hEffMapN[Channel * 10 + iroc] = new TH2F(Name, Name, PLTU::NCOL, PLTU::FIRSTCOL, PLTU::LASTCOL + 1, PLTU::NROW, PLTU::FIRSTROW, PLTU::LASTROW + 1);
          Name = TString::Format("EffNumeratorSlopeX_Ch%i_ROC%i", Channel, iroc);
	        hEffMapSlopeXN[Channel * 10 + iroc] = new TH1F(Name, Name, 100, -0.1, 0.1);
          Name = TString::Format("EffNumeratorSlopeY_Ch%i_ROC%i", Channel, iroc);
          hEffMapSlopeYN[Channel * 10 + iroc] = new TH1F(Name, Name, 100, -0.1, 0.1);
          Name = TString::Format("EffNumeratorPulseHeight_Ch%i_ROC%i", Channel, iroc);
          hEffMapPulseHeightN[Channel * 10 + iroc] = new TH1F(Name, Name, 60, -1000, 50000);
          Name = TString::Format("EffDenominator%i_ROC%i", Channel, iroc);
          hEffMapD[Channel * 10 + iroc] = new TH2F(Name, Name, PLTU::NCOL, PLTU::FIRSTCOL, PLTU::LASTCOL + 1, PLTU::NROW, PLTU::FIRSTROW, PLTU::LASTROW + 1);
          Name = TString::Format("EffDenominatorSlopeX_Ch%i_ROC%i", Channel, iroc);
          hEffMapSlopeXD[Channel * 10 + iroc] = new TH1F(Name, Name, 100, -0.1, 0.1);
          Name = TString::Format("EffDenominatorSlopeY_Ch%i_ROC%i", Channel, iroc);
          hEffMapSlopeYD[Channel * 10 + iroc] = new TH1F(Name, Name, 100, -0.1, 0.1);
          Name = TString::Format("EffDenominatorPulseHeight_Ch%i_ROC%i", Channel, iroc);
          hEffMapPulseHeightD[Channel * 10 + iroc] = new TH1F(Name, Name, 60, -1000, 50000);
          Name = TString::Format("ExtrapolatedTrackPulseHeights_Ch%i_ROC%i", Channel, iroc);
          hMapPulseHeights[Channel * 10 + iroc] = new TH1F(Name, Name, 60, -1000, 50000);
        }
      }

      PLTPlane* Plane[3] = {0x0, 0x0, 0x0};
      for (size_t ip = 0; ip != NPlanes; ++ip) {
        Plane[ Telescope->Plane(ip)->ROC() ] = Telescope->Plane(ip);
      }

      // To construct 4 tracks.. one testing each plane.. and one using all planes if it be.
      PLTTrack Tracks[4];

      // If it has all 3 planes that'll be number 0
      if (Plane[0]->NClusters() && Plane[1]->NClusters() && Plane[2]->NClusters()) {
        Tracks[0].AddCluster(Plane[0]->Cluster(0));
        Tracks[0].AddCluster(Plane[1]->Cluster(0));
        Tracks[0].AddCluster(Plane[2]->Cluster(0));
        Tracks[0].MakeTrack(Alignment);
      }

      // 2-plane tracks
      if (Plane[0]->NClusters() && Plane[1]->NClusters()) {
        Tracks[1].AddCluster(Plane[0]->Cluster(0));
        Tracks[1].AddCluster(Plane[1]->Cluster(0));
        Tracks[1].MakeTrack(Alignment);
      }
      if (Plane[0]->NClusters() && Plane[2]->NClusters()) {
        Tracks[2].AddCluster(Plane[0]->Cluster(0));
        Tracks[2].AddCluster(Plane[2]->Cluster(0));
        Tracks[2].MakeTrack(Alignment);
      }
      if (Plane[1]->NClusters() && Plane[2]->NClusters()) {
        Tracks[3].AddCluster(Plane[1]->Cluster(0));
        Tracks[3].AddCluster(Plane[2]->Cluster(0));
        Tracks[3].MakeTrack(Alignment);
      }

      //fill slopes for 3 fold coincidences
      bool roc0Good = Tracks[0].IsFiducial(Channel,0,Alignment,FidRegionTrack);
      bool roc1Good = Tracks[0].IsFiducial(Channel,1,Alignment,FidRegionTrack);
      bool roc2Good = Tracks[0].IsFiducial(Channel,2,Alignment,FidRegionTrack);

      //bool isGoodTrack = Tracks[0].NHits()>2;

      // std::cout << "Event# " << ientry << std::endl;
      // std::cout << roc0Good << " " << roc1Good << " " << roc2Good << " " << Tracks[0].NHits() << std::endl;

      if ( roc0Good && roc1Good && roc2Good && (Tracks[0].NClusters()>2) ){
          float slopeX = Tracks[0].fTVX/Tracks[0].fTVZ;
          float slopeY = Tracks[0].fTVY/Tracks[0].fTVZ;
          hSlopeX[Channel]->Fill(slopeX);
          hSlopeY[Channel]->Fill(slopeY);
          hSlopeXvsSlopeY[Channel]->Fill(slopeX,slopeY);
          float slopeY_transformed = transformSlopeY(slopeX, slopeY);
          hSlopeXvsSlopeY_Transformed[Channel]->Fill(slopeX,slopeY_transformed);
      }

      // Test of plane 2
      if (Plane[0]->NClusters() && Plane[1]->NClusters()) {
        if (Tracks[1].IsFiducial(Channel, 2, Alignment, FidRegionTrack) && Tracks[1].NHits() == 2) {
          ++HC[Channel].NFiducial[2];
          PLTAlignment::CP* CP = Alignment.GetCP(Channel, 2);
          std::pair<float, float> LXY = Alignment.TtoLXY(Tracks[1].TX( CP->LZ ), Tracks[1].TY( CP->LZ ), Channel, 2);
          std::pair<int, int> PXY = Alignment.PXYfromLXY(LXY);
          hEffMapD[Channel * 10 + 2]->Fill(PXY.first, PXY.second);
          hEffMapSlopeXD[Channel * 10 + 2]->Fill(Tracks[0].fTVX/Tracks[0].fTVZ);
          hEffMapSlopeYD[Channel * 10 + 2]->Fill(Tracks[0].fTVY/Tracks[0].fTVZ);
	  float cluster_charge = 0;
	  if(Plane[2]->NClusters()) cluster_charge=Plane[2]->Cluster(0)->Charge();
          hEffMapPulseHeightD[Channel * 10 + 2]->Fill(cluster_charge);
          if (Plane[2]->NClusters() > 0) {
            std::pair<float, float> ResXY = Tracks[1].LResiduals( *(Plane[2]->Cluster(0)), Alignment );
            std::pair<float, float> const RPXY = Alignment.PXYDistFromLXYDist(ResXY);
            //printf("ResXY: %15.9f   RPXY: %15.9f\n", ResXY.first, RPXY.first);
            if (abs(RPXY.first) <= PixelDist && abs(RPXY.second) <= PixelDist) {
              //hEffMapN[Channel * 10 + 2]->Fill(Plane[2]->Cluster(0)->SeedHit()->Column(), Plane[2]->Cluster(0)->SeedHit()->Row());
              hEffMapN[Channel * 10 + 2]->Fill(PXY.first, PXY.second);
	      hEffMapSlopeXN[Channel * 10 + 2]->Fill(Tracks[0].fTVX/Tracks[0].fTVZ);
	      hEffMapSlopeYN[Channel * 10 + 2]->Fill(Tracks[0].fTVY/Tracks[0].fTVZ);
	      hEffMapPulseHeightN[Channel * 10 + 2]->Fill(cluster_charge);
	      hMapPulseHeights[Channel * 10 + 2]->Fill(cluster_charge);
              ++HC[Channel].NFiducialAndHit[2];
            } else {
              static int ievent = 0;
              if (ievent < 20) {
                Telescope->DrawTracksAndHits( TString::Format("plots/Jeebus_%04i.gif", ievent++).Data());
              }
	      hMapPulseHeights[Channel * 10 + 2]->Fill(0);
            }
          }
        }
      }

      // Test of plane 1
      if (Plane[0]->NClusters() && Plane[2]->NClusters()) {
        if (Tracks[2].IsFiducial(Channel, 1, Alignment, FidRegionTrack) && Tracks[2].NHits() == 2) {
          ++HC[Channel].NFiducial[1];
          PLTAlignment::CP* CP = Alignment.GetCP(Channel, 1);
          std::pair<float, float> LXY = Alignment.TtoLXY(Tracks[2].TX( CP->LZ ), Tracks[2].TY( CP->LZ ), Channel, 1);
          std::pair<int, int> PXY = Alignment.PXYfromLXY(LXY);
          hEffMapD[Channel * 10 + 1]->Fill(PXY.first, PXY.second);
          hEffMapSlopeXD[Channel * 10 + 1]->Fill(Tracks[2].fTVX/Tracks[2].fTVZ);
          hEffMapSlopeYD[Channel * 10 + 1]->Fill(Tracks[2].fTVY/Tracks[2].fTVZ);
	  float cluster_charge = 0;
	  if(Plane[1]->NClusters()) cluster_charge=Plane[1]->Cluster(0)->Charge();
          hEffMapPulseHeightD[Channel * 10 + 1]->Fill(cluster_charge);
          if (Plane[1]->NClusters() > 0) {
            std::pair<float, float> ResXY = Tracks[2].LResiduals( *(Plane[1]->Cluster(0)), Alignment );
            std::pair<float, float> const RPXY = Alignment.PXYDistFromLXYDist(ResXY);
            if (abs(RPXY.first) <= PixelDist && abs(RPXY.second) <= PixelDist) {
              hEffMapN[Channel * 10 + 1]->Fill(PXY.first, PXY.second);
	      hEffMapSlopeXN[Channel * 10 + 1]->Fill(Tracks[2].fTVX/Tracks[2].fTVZ);
	      hEffMapSlopeYN[Channel * 10 + 1]->Fill(Tracks[2].fTVY/Tracks[2].fTVZ);
	      hEffMapPulseHeightN[Channel * 10 + 1]->Fill(cluster_charge);
	      hMapPulseHeights[Channel * 10 + 1]->Fill(cluster_charge);
              ++HC[Channel].NFiducialAndHit[1];
            } else hMapPulseHeights[Channel * 10 + 1]->Fill(0);
          }
        }
      }

      // Test of plane 0
      if (Plane[1]->NClusters() && Plane[2]->NClusters()) {
        if (Tracks[3].IsFiducial(Channel, 0, Alignment, FidRegionTrack) && Tracks[3].NHits() == 2) {
          ++HC[Channel].NFiducial[0];
          PLTAlignment::CP* CP = Alignment.GetCP(Channel, 0);
          std::pair<float, float> LXY = Alignment.TtoLXY(Tracks[3].TX( CP->LZ ), Tracks[3].TY( CP->LZ ), Channel, 0);
          std::pair<int, int> PXY = Alignment.PXYfromLXY(LXY);
          hEffMapD[Channel * 10 + 0]->Fill(PXY.first, PXY.second);
          hEffMapSlopeXD[Channel * 10 + 0]->Fill(Tracks[3].fTVX/Tracks[3].fTVZ);
          hEffMapSlopeYD[Channel * 10 + 0]->Fill(Tracks[3].fTVY/Tracks[3].fTVZ);
	  float cluster_charge = 0;
	  if(Plane[0]->NClusters()) cluster_charge=Plane[0]->Cluster(0)->Charge();
          hEffMapPulseHeightD[Channel * 10 + 0]->Fill(cluster_charge);
          if (Plane[0]->NClusters() > 0) {
            std::pair<float, float> ResXY = Tracks[3].LResiduals( *(Plane[0]->Cluster(0)), Alignment );
            std::pair<float, float> const RPXY = Alignment.PXYDistFromLXYDist(ResXY);
            if (abs(RPXY.first) <= PixelDist && abs(RPXY.second) <= PixelDist) {
              hEffMapN[Channel * 10 + 0]->Fill(PXY.first, PXY.second);
	      hEffMapSlopeXN[Channel * 10 + 0]->Fill(Tracks[3].fTVX/Tracks[3].fTVZ);
	      hEffMapSlopeYN[Channel * 10 + 0]->Fill(Tracks[3].fTVY/Tracks[3].fTVZ);
	      hEffMapPulseHeightN[Channel * 10 + 0]->Fill(cluster_charge);
	      hMapPulseHeights[Channel * 10 + 0]->Fill(cluster_charge);
              ++HC[Channel].NFiducialAndHit[0];
            } else hMapPulseHeights[Channel * 10 + 0]->Fill(0);
          }
        }
      }

      //printf("%9i %9i %9i\n", HC[Channel].NFiducialAndHit[0], HC[Channel].NFiducialAndHit[1], HC[Channel].NFiducialAndHit[2]);

    }
  }


  // Save some efficiency maps
  for (std::map<int, TH2F*>::iterator it = hEffMapD.begin(); it != hEffMapD.end(); ++it) {
    int const id = it->first;
    int const Channel = id / 10;
    int const ROC     = id % 10;

    TString Name = TString::Format("TrackingEfficiencyMap_Ch%i_ROC%i", Channel, ROC);
    TCanvas Can1(Name, Name, 400, 1200);
    Can1.Divide(1, 3);

    Can1.cd(1);
    hEffMapN[id]->SetTitle(Name);
    hEffMapN[id]->Divide(it->second);
    hEffMapN[id]->SetMinimum(0.3);
    hEffMapN[id]->SetMaximum(1.7);
    hEffMapN[id]->SetStats(0);
    hEffMapN[id]->Draw("colz");

    Can1.cd(2)->SetLogy();
    TH1F* Hist1D = FidHistFrom2D(hEffMapN[id], "", 30, FidRegionTrack);
    Hist1D->SetTitle( TString::Format("Tracking Efficiency Ch%i ROC%i", Channel, ROC));
    Hist1D->SetXTitle("Efficiency");
    Hist1D->SetYTitle("# of Pixels");
    Hist1D->SetMinimum(0.5);
    Hist1D->Clone()->Draw();

    Can1.cd(3);
    Hist1D->SetMinimum(0);
    Hist1D->Draw();

    Name = TString::Format("plots/TrackingEfficiencyMap_Ch%i_ROC%i", Channel, ROC);
    Can1.SaveAs(Name + ".gif");

    Name = TString::Format("TrackingEfficiencySlopes_Ch%i_ROC%i", Channel, ROC);
    TCanvas Can2(Name, Name, 600, 1200);
    Can2.Divide(1, 3);

    Can2.cd(1);
    Name = TString::Format("Tracking Efficiency vs. Slope X for Ch%i ROC%i", Channel, ROC);
    hEffMapSlopeXN[id]->SetTitle(Name);
    hEffMapSlopeXN[id]->Divide(hEffMapSlopeXD[id]);
    hEffMapSlopeXN[id]->GetYaxis()->SetTitle("Tracking Efficiency");
    hEffMapSlopeXN[id]->GetXaxis()->SetTitle("Local Telescope Track-SlopeX #DeltaX/#DeltaZ");
    hEffMapSlopeXN[id]->SetStats(0);
    hEffMapSlopeXN[id]->Draw();

    Can2.cd(2);
    Name = TString::Format("Tracking Efficiency vs. Slope Y for Ch%i ROC%i", Channel, ROC);
    hEffMapSlopeYN[id]->SetTitle(Name);
    hEffMapSlopeYN[id]->Divide(hEffMapSlopeYD[id]);
    hEffMapSlopeYN[id]->GetYaxis()->SetTitle("Tracking Efficiency");
    hEffMapSlopeYN[id]->GetXaxis()->SetTitle("Local Telescope Track-SlopeY #DeltaY/#DeltaZ");
    hEffMapSlopeYN[id]->SetStats(0);
    hEffMapSlopeYN[id]->Draw();

    Can2.cd(3);
    Name = TString::Format("Tracking Efficiency vs. Pulse Height for Ch%i ROC%i", Channel, ROC);
    hEffMapPulseHeightN[id]->SetTitle(Name);
    hEffMapPulseHeightN[id]->Divide(hEffMapPulseHeightD[id]);
    hEffMapPulseHeightN[id]->GetYaxis()->SetTitle("Tracking Efficiency");
    hEffMapPulseHeightN[id]->GetXaxis()->SetTitle("Electrons");
    hEffMapPulseHeightN[id]->SetStats(0);
    hEffMapPulseHeightN[id]->Draw();

    Name = TString::Format("plots/TrackingEfficiencySlopes_Ch%i_ROC%i", Channel, ROC);
    Can2.SaveAs(Name + ".gif");

    Name = TString::Format("ExtrapolatedTrackPulseHeights_Ch%i_ROC%i", Channel, ROC);
    TCanvas Can3(Name, Name, 400, 400);

    Can3.cd(1);
    Name = TString::Format("Extrapolated Track Pulse Heights for Ch%i ROC%i", Channel, ROC);
    hMapPulseHeights[id]->SetTitle(Name);
    //hEffMapSlopeXN[id]->GetYaxis()->SetTitle("Events per");
    hMapPulseHeights[id]->GetXaxis()->SetTitle("Electrons");
    hMapPulseHeights[id]->SetFillColor(40);
    gStyle->SetOptStat(111111);
    hMapPulseHeights[id]->Draw();
    
    Name = TString::Format("plots/ExtrapolatedTrackPulseHeights_Ch%i_ROC%i", Channel, ROC);
    Can3.SaveAs(Name + ".gif");

  }

  //plot all the slopes
  TFile *slopeFile = new TFile("slopePlots.root","RECREATE");
  for (std::map<int, TH1F*>::iterator it = hSlopeX.begin(); it != hSlopeX.end(); ++it) {

    int channel = it->first;
    TCanvas Can1("Can1","", 1000, 500);
    Can1.Divide(2, 2);
    hSlopeX[channel]->GetXaxis()->SetTitle("Local Telescope Track-SlopeX #DeltaX/#DeltaZ");
    hSlopeY[channel]->GetXaxis()->SetTitle("Local Telescope Track-SlopeY #DeltaY/#DeltaZ");
    hSlopeXvsSlopeY[channel]->GetXaxis()->SetTitle("Local Telescope Track-SlopeX #DeltaX/#DeltaZ");
    hSlopeXvsSlopeY_Transformed[channel]->GetXaxis()->SetTitle("Local Telescope Track-SlopeX #DeltaX/#DeltaZ");

    hSlopeX[channel]->GetYaxis()->SetTitle("Number of Tracks");
    hSlopeY[channel]->GetYaxis()->SetTitle("Number of Tracks");
    hSlopeXvsSlopeY[channel]->GetYaxis()->SetTitle("Local Telescope Track-SlopeY #DeltaY/#DeltaZ");
    hSlopeXvsSlopeY_Transformed[channel]->GetYaxis()->SetTitle("Local Telescope Track-SlopeY #DeltaY/#DeltaZ");

    hSlopeX[channel]->GetYaxis()->SetTitleOffset(1.3);
    hSlopeY[channel]->GetYaxis()->SetTitleOffset(1.3);

    // hSlopeX[channel]->GetXaxis()->SetNdivisions(11);
    // hSlopeY[channel]->GetXaxis()->SetNdivisions(11);

    hSlopeX[channel]->SetStats(1111);
    hSlopeY[channel]->SetStats(1111);
    hSlopeXvsSlopeY[channel]->SetStats(1111);
    hSlopeXvsSlopeY_Transformed[channel]->SetStats(1111);

    Can1.cd(1);
    hSlopeX[channel]->Draw();

    Can1.cd(2);
    hSlopeY[channel]->Draw();

    Can1.cd(3);
    hSlopeXvsSlopeY[channel]->Draw("colz");

    Can1.cd(4);
    hSlopeXvsSlopeY_Transformed[channel]->Draw("colz");

    slopeFile->cd();
    hSlopeX[channel]->Write();
    hSlopeY[channel]->Write();
    hSlopeXvsSlopeY[channel]->Write();
    hSlopeXvsSlopeY_Transformed[channel]->Write();

    TString Name = TString::Format("plots/TrackSlopes_Ch%i",channel);
    Can1.SaveAs(Name+".gif");
    // if(channel == 9){
    //   int leftbin = hSlopeY[9]->FindBin(-0.005)-1;
    //   int rightbin = hSlopeY[9]->FindBin(0.002)+1;
    //   double numAccidentals = 1.0*( hSlopeY[9]->Integral(0,leftbin) + hSlopeY[9]->Integral(rightbin,hSlopeY[9]->GetNbinsX()+1) );
    //   double total = 1.0*hSlopeY[9]->Integral(0,hSlopeY[9]->GetNbinsX()+1);
    //   double rate = numAccidentals/total;
    //   double err = sqrt( rate*(1.0-rate)/total );
    //   std::cout << "Accidental rate: " << rate << " +- " << err << std::endl;
    //   std::cout << "Total tracks: " << total << std::endl;
    // }


  }
  slopeFile->Close();
  delete slopeFile;

  for (std::map<int, HitCounter>::iterator it = HC.begin(); it != HC.end(); ++it) {
    printf("Efficiencies for Channel %2i:\n", it->first);
    for (int i = 0; i != 3; ++i) {
      printf("ROC %1i  NFiducial: %10i  NFiducialAndHit: %10i  Efficiency: %12.9f\n",
             i, it->second.NFiducial[i],
             it->second.NFiducialAndHit[i],
             float(it->second.NFiducialAndHit[i]) / float(it->second.NFiducial[i]) );
    }
  }

  return 0;
}


int main (int argc, char* argv[])
{
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " [DataFile.dat] [GainCal.dat] [AlignmentFile.dat]" << std::endl;
    return 1;
  }

  std::string const DataFileName = argv[1];
  std::string const GainCalFileName = argv[2];
  std::string const AlignmentFileName = argv[3];

  TrackingEfficiency(DataFileName, GainCalFileName, AlignmentFileName);

  return 0;
}
