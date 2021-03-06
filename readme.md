
This directory contains a package for rectangular cut optimization
with TMVA. Scroll down for step by step instructions.

# Files and their content

- Variables.hh: This file contains the list of the variables
     for TMVA optimization and the list of spectators. The
     list of variables is structured so that it contains the
     variable name exactly as in the input ntuple, the variable
expression as it is passed to TMVA (such as "abs(d0)"),

- VariableLimits.hh: This file defines sets of user-imposed limits
     for cut optimization. One may want to, e.g., enforce that
     the H/E cut is no looser than 0.15 for whatever reason, or 
     one may want to make some of the cuts to be no looser than 
     HLT cuts. These limitations are defined here, and passed
     as a parameter into optimize.cc code from a higher-level script.

- OptimizationConstants.hh: this file contains about all settings
     for optimization (except for the variable list): input files/trees, 
     working points specs, TMVA options, etc.

- VarCut.hh/.cc: The class that contains a single set of cut values.
     It is a writable ROOT object. It has the same number of
     variables as specified in Variables.hh, and one should refer
     to Variables.hh to find out which variable has which name.

- optimize.hh/.cc: This is not a class, but plaine code with the main
     function optimize(...). It runs a single optimization of rectangular cuts.
     The parameters passed in:
        - the location of the ROOT file with VarCut object that defines
             the limits for cuts during optimization
        - the base for the Root file name construction for the output VarCut
             objects for working points.
        - the base for dir and file names for the standard TMVA output
        - one of the predefined sets of user-defined cut restrictions.
           During optimization, the code chooses the tightest restriction
           out of those imposed from the cut file passed as the first parameter
           to this function above (usually 99.9% or the previous working point)
           and these user-predefined cut restrictions (see VariableLimits.hh).

- rootlogon.C: automatically builds and loads several pieces of code
     such as VarCut.cc, etc.

- simpleOptimization.C: runs simple one-pass optimization calling optimize().
     Presently, it is suggested to run this code without compiling
     (it compiles, but on exit ROOT gives segv, most likely while trying
     to delete factories).
        The output cuts for working points are found in the cut_repository/
     subdirectory with the names configured in the code.

- fourPointOptimization.C: runs optimization in four passes. The first
     pass uses 99.9% efficient cut range for optimization, the second
     uses WP Veto cuts as cut limits, the third uses WP Loose as cut
     limits, etc.
       In addition to using the working point of the previous pass for the
     next pass as cut limits, additionally user-defined restrictions are passed
     to the optimize() function. These are defined in VariableLimits.hh. Presently,
     for WP Veto in pass1, the object with effectively no restrictions is passed,
     while for pass2,3,4 the object with reasonable common sense restrictions is
     passed.
       In the end, WP Veto, Loose, Medium, Tight are taken from the
     pass1, pass2, pass3, pass4 output, respectively.
        The output cuts for working points are found in the cut_repository/
     subdirectory with the names configured in the code.
       

- fillCuts.py: an example of how to create ROOT files with VarCut objects
     if cuts are known from somewhere else.

- findCutLimits.C: this code determines cut values that correspond to
     99.9% efficiency for each variable separately. It uses all definitions
     the same as the optimization: what is the preselection, what is the
     signal ntuple, where to write cut object filled with 99.9% efficient cuts,
     etc. The unique part of the cut file name is defined by the dateTag
     string in the beginning of the file and should be changed as needed.
     Note: the "sensible limits" for internal machinery are set for
     the present electron variables in findVarLimits(..) function and
     need to be updated if other variables are added.

- computeSingleCutEfficiency.C: this script computes the signal and background
     efficiency of a single cut for a variable from the list defined in Variables.hh 
     using preselection defined in OptimizationConstants.hh.
     Use it as follows:
        .L computeSingleCutEfficiency.C+
        bool forBarrel = true;
        float cutValue = 0.2;
        computeSingleCutEfficiency("d0",cutValue, forBarrel);

- drawVariablesAndCuts.C: this script draws distributions of all variables
     and if requested also draws cuts corresponding to four working points.
     The behavior is controlled by global parameters in the beginning of
     the script (barrel or endcap, which cuts to draw, etc). Some of the info
     is taken from OptimizationConstants.hh, but the names of the ntuples
     are inside of this script. Compile and run it without parameters.

- drawROCandWP.py: this script draws the ROC and up to three sets of 
     working points.
     - The ROC is taken from the specified TMVA files
     - Each set of cuts is taken from the list of files defined in common.workingpoints
     - The efficiencies for working points are computed right in this script,
      using the signal and background ntuples specfied explicitly in the beginning
      of the file and preselection cuts taken from OptimizationConstants.hh

- drawKinematics.py: the script draws unweighted and weighted pt and eta distributions.

- drawEfficiency.py: the script draws efficiencies for all working points as a function
      of pt, eta, Nvtx.
     
- correlations.C and tmvaglob.C: these are the standard pieces of code that come
      from TMVA examples directory without any changes. To draw correlations, do:
      .L correlations.C
      correlations("path/to/your/TMVA.root");

- convert_EventStrNtuple_To_FlatNtuple.C: converts event-structured ntuple to
      the flat ntuple for ID tuning

- computeHLTBounds.C: applies UCCOM method to find the offline cut bounds on isolation
      from the HLT cuts and variables. Requires a special ntuple that contains
      HLT-like ecal/hcal/trk isolations.

- repackageCuts.C: this script loads VarCut objects with cuts, and changes some of them,
      those that are adjusted by hand. The script writes out new files with adjusted
      VarCut objects.



Directories:
  
- cut_repository/: created to contain ROOT files with individual cut sets
      saved as VarCut objects.

- trainingData/: created to contain subdirectories with the standard
      output of TMVA (weights xml, ROOT file with training diagnostics).
      WARNING: this directory can become large if not cleaned up
      occasionally, because TMVA usually saves full testing and training
      trees.





# Usage

To run cut optimization, first look through contents of the
OptimizationConstants.hh and Variables.hh and adjust as necessary
for your case. At the very least change input file and tree names
and the numbers of train and test events for TMVA.

To run simple one-pass optimization:

root -b -q simpleOptimization.C >& test.log &
tail -f test.log

To run full four-pass optimization for four working points:

root -b -q fourPointOptimization.C >& test.log &
tail -f test.log

The output for the working points is found in the cut_repository/

To create a ROOT file with a stored cut set: edit cut values in
exampleFillCuts.C, and then run:
  root -b -q exampleFillCuts.C+
and the files with cuts will appear in the cut_repository/.



# Electron ID tuning steps for Run III studies

## 1. 
Ntuples are made for the DY, TT, GJ datasets (the last one is 
just for comparison plots). The effective area does not have to be
up to date for the ntuple maker. The ntuples are made with (c++, python, crab):

```
cmsrel CMSSW_10_6_2
cd CMSSW_10_6_2/src/
cmsenv
git clone https://github.com/cms-egamma/EgammaWork.git
cd EgammaWork/
git checkout ntupler_106X
scram build -j 10
```

The cmsRun executables are in EgammaWork/ElectronNtupler/test:
runElectrons_AOD.py
runElectrons.py
(they only differ by a boolean)

Submit them using crabSubmit.py


## 2. 
Effective areas are prepared with the ElectronWork/EffectiveAreas
package, see https://github.com/tomcornelis/ElectronWork

*todo:* move the relevant computeEffectiveAreaWithIsoCutoffs script to this package


## 3. 
Since 2016, we are reweighting signal (DY) to background (TTbar)
in 2D, pt and eta. Weights need to be prepared. To do this,
edit the beginning of the script (file names), change the datetime tag at the bottom of the script,
and run it like this once:

```
./compileAndRun.sh computeKinematicWeights
```


## 4. 
Convert the full ntuples with event structure into much reduced flat (one entry
is an electron, not an event with vectors) ntuples for TMVA. The smaller are the final 
ntuples,  the faster is the tuning. What was done so far is to create one flat
ntuple for each of the following: signal barrel, signal endcap, background barrel,
background endcap, these are iterated over in the main function at the bottom of the script.

The script needs the correct tagDir specified in order to obtain the kinematic
reweighting file from step 3 and to know where to store the trees.
This script also requires numerical values for the final effective areas, in the beginning of the code (see step 2).
Note that this script ignores the "relIsoWithEA" computed in the original ntuple, and
replaces it what is computed on the spot from the PF isolations of three types, 
the rho and the effective areas hardcoded in this file. 

NOTE: the kinematic weights are set to meaningful values only for the sample DY and matching choice TRUE. For any
other combination of flags set in the beginning of the convert... script, kinematic
weights are 1 for all events.

```
./compileAndRun.sh convert_EventStrNtuple_To_FlatNtuple
```


## 5. 
Edit the file that contain the varialbles that will go into tuning (if needed):

  Variables.hh

For this tuning, we have 6 variables that go into TMVA tuning, and the rest
are spectators.

Further, edit the file that contain the majority of settings for TMVA tuning, but
is also used by other scripts.

  OptimizationConstants.hh

Review here: 
  - numbers of events to train/test on (500K was sufficient for training
        in the past, but one should watch out for the ROC becoming clearly 
	non-smooth, and increase the sample in that case. This may happen
        due to various weights being used and reducing effective statistics)
  - file names for the input flat signal/background ntuples created in the
       step above


## 6. 
Compute limits within which TMVA will look for optimal cuts, aiming
for 99.9% efficient range for each variable of the ID. The script requires
only the change of the name string in the beginning. It takes all other
info from header files with constants, such as Variables.hh and OptimizationConstants.hh.
Run it as:
```
  root -q -b findCutLimits.C++
```
The output of the script is printed on the screen, and is also saved 
in ROOT files with names like:

  cut_repository/cuts_barrel_eff_0999_DATETAG.root
  cut_repository/cuts_endcap_eff_0999_DATETAG.root

These files will be used directly in the steps below.


## 7. 
Run optimization of four working points.

 Before doing that, check that OptimizationConstants.hh have the 
desired settings. In the first attempt, one may want to do a quick
(sort of) test with few events, like 20K training size would take maybe
30min+. Then restore the settings to the full training size, such as 300K.
  Also, check the settings in the VariableLimits.hh. The standard
first order behavior of the optimization is to have for each variable
the range that is from zero to 99.9% efficient for the working point Veto,
then from then on the range is from zero to the cut value of the previous
working point (for Loose, Medium, Tight). The VariableLimits allows
one to put further restrictions, and TMVA gets an OR of the previous working
point (or 99.9% efficient value) and the user defined restrictions. In 2016
this was used, for example, to make sure that some working points are tighter
than HLT.

 In the fourPointOptimization.C, check:
 - the datetag is consistent with the former steps
 - check that the loop goes over four passes (sometimes the script is
      modified to run only on some passes, and the setting is forgotten).
 - check that "user defined cut limits" do what is needed. In 2016,
      these extra limits were used to make some working points tighter
      than HLT. See above comments for VariableLimits.hh
```
  root -b -q 'fourPointOptimization.C(true)'  &> barrel.log &
  root -b -q 'fourPointOptimization.C(false)' &> endcap.log &
```

One can prepend the root command with "nohup" and come and check
it next day, since it may take awhile. On lxplus this might not work,
and the job may be gone without completing.
   One should always save logs from at least final optimizations to 
be able to review them later in case of any issues.

Each optimization will take awhile: each has four passes, so one will see
the progress bar four times. The four passes are necessary to make
each next working point tighter in all cuts than the previous working point.

If there is a crash, or working points do not look good, one can
attempt to re-do the optimization with misc adjustments, such as:
  - extra restrictions on the range of some of the ID variables in VariableLimits.hh
  - change in the number of training events in OptimizationConstants.hh
One can re-do the otimization of the failed and subsequent passes
only by changing the starting pass in the loop over passes in the fourPointOptimization.C.

*Optimization results*
At the end of the run of the optiization script the tuned working
points are printed on the screen. However, this is just for general
information. The saved outcome discussed below is what is really useful.

The results of the optimization are saved in a few different ways.
The standard output of TMVA goes into a separate directory for each 
pass, such as:

trainingData/training_results_barrel_pass1_DATETAG
trainingData/training_results_barrel_pass2_DATETAG
trainingData/training_results_barrel_pass3_DATETAG
trainingData/training_results_barrel_pass4_DATETAG

the ROOT file there contains the ROCs and the training and testing
trees used in the tuning.

The cuts of the working points are saved in the cuts_repository/ directory.
Here is an example of a single run of fourPointOptimization.C:

cut_repository/cuts_barrel_pass1_DATETAG_WP_Veto.root
cut_repository/cuts_barrel_pass1_DATETAG_WP_Loose.root
cut_repository/cuts_barrel_pass1_DATETAG_WP_Medium.root
cut_repository/cuts_barrel_pass1_DATETAG_WP_Tight.root
cut_repository/cuts_barrel_pass2_DATETAG_WP_Veto.root
cut_repository/cuts_barrel_pass2_DATETAG_WP_Loose.root
cut_repository/cuts_barrel_pass2_DATETAG_WP_Medium.root
cut_repository/cuts_barrel_pass2_DATETAG_WP_Tight.root
cut_repository/cuts_barrel_pass3_DATETAG_WP_Veto.root
cut_repository/cuts_barrel_pass3_DATETAG_WP_Loose.root
cut_repository/cuts_barrel_pass3_DATETAG_WP_Medium.root
cut_repository/cuts_barrel_pass3_DATETAG_WP_Tight.root
cut_repository/cuts_barrel_pass4_DATETAG_WP_Veto.root
cut_repository/cuts_barrel_pass4_DATETAG_WP_Loose.root
cut_repository/cuts_barrel_pass4_DATETAG_WP_Medium.root
cut_repository/cuts_barrel_pass4_DATETAG_WP_Tight.root
cut_repository/cuts_barrel_DATETAG_WP_Veto.root
cut_repository/cuts_barrel_DATETAG_WP_Loose.root
cut_repository/cuts_barrel_DATETAG_WP_Medium.root
cut_repository/cuts_barrel_DATETAG_WP_Tight.root

Above list has four working points created after each pass.
The final working points are the ones that contain the same date 
stamp by no word "passX" in the name. The Veto is taken from pass1,
the Loose is from pass2, Medium from pass3, Tight from pass4.

The cuts are stored in the form of a storable VarCut object defined
in this package. To view a cut, one can start a ROOT session
while being in the main directory of the package (so that the rootlogon.C
loads all libraries) and execute, e.g.L

  TFile f("cut_repository/cuts_barrel_DATETAG_WP_Tight.root")
  .ls
  cuts->Print()


## 8. 
Review the optimization results. Note that all the scripts here
produce plots that can go straight into the presentations.

#### a)
Draw pt and eta spectra of the signal and background electrons,
with and without reweighting. This script uses the flat ntuples
that contain both barrel and endcap (i.e. "alleta").
Change the datetag and run:
```
./drawKinematics.py
```

#### b) 
Draw ROC and display the working points in the ROC space. In the
script that does this one needs to specify locations of the TMVA output
for all four passes, as well as the files with the final cut objects.
Typically one just needs to edit in the file the date string, the rest
of the names are standard. 
   Note that the ROC is built piece-wise out of four ROCs, one from
each pass. This way, each working point gets its own native ROC segment.
For example, the working point Medium with 80% target efficiency is taken 
from the pass2, and the ROC displayed contains the segment of the ROC from
pass2 between 75 and 90% of the signal efficiency.
   The markers are drawn based on the TestTree from the TMVA file, the
same tree which (to our best guess) used to build the ROCs, so the WP markers
should really be sitting on the final composite ROC. 
   Note that plots are made with and without an extra cut 
that is not applied in TMVA preselection and not tuned by the TMVA:
the cut on the expected missing inner hits.

  The script computes the efficiencies for the signal with exactly the same
weights that TMVA used to tune the cuts.
First add the workingpoints to common.py and then modify drawROCandWPv4.py
in order to plot these new workingpoints.
```
./drawROCandWPv4.py
```

The output, a presentation-grade figure, is saved into a file with the 
name like

   figures/plot_ROCandWP_barrel.png
   figures/plot_ROCandWP_barrel_noMissingHits.png

Typically you show the second plot which does not include the missing hits
cut such that the workingpoints are perfectly overlayed with the ROC curve.

#### c) 
Draw distributions of all ID variables. This script draws distributions
of all ID variables and overlays cuts for all working points. 
In this script one has to set up the names of all relevant flat ntuples
and the barrel/endcap flag. The drawn variables include both the variables
tuned by TMVA and other ID variables where the cut is chosen manually
(thus these manual cuts need to be reviewed during tuning). At this 
moment the "mannual" cuts include: d0, dz, and the expected missing inner hits.

   The script draws fake electron background from three sources for each
variable: from the TTJets sample (which is what was used in the TMVA tunning,
and contains both jets faking electrons and secondary electrons from heavy flavor),
from the DYJetsToLL sample (which is mostly light jets faking electrons) and
from the GJet_something sample (which is jets or energetic photons faking
electrons).

   Run this script as (for barrel and endcap respectively):
```
root -b -q 'drawVariablesAndCuts_3bg.C(true)'
root -b -q 'drawVariablesAndCuts_3bg.C(false)'
```

The output of the script, plots for a presentation, goes to files like:

```
   figures/plot_barrel_3BGs_full5x5_sigmaIetaIeta.png
   figures/plot_barrel_3BGs_dEtaSeed.png
   figures/plot_barrel_3BGs_dPhiIn.png
   figures/plot_barrel_3BGs_hOverE.png
   figures/plot_barrel_3BGs_relIsoWithEA.png
   figures/plot_barrel_3BGs_ooEmooP.png
   figures/plot_barrel_3BGs_d0.png
   figures/plot_barrel_3BGs_dz.png
   figures/plot_barrel_3BGs_expectedMissingInnerHits.png
```
Note that some distributions might be better displayed in the log-Y mode.
For that, one can change the script or just change it interactively on
the desired canvas and save the figure manually.

#### d) 
Draw the ID efficiencies as a function of pt, eta, and the number
of vertices. At the bottom of the drawEfficiency.py script, a loop is
made over the different x-axis options:
 - eta
 - pt
 - generator pt
 - pt extended range up to 2 TeV (needs a Flat DoubleElectron sample)
 - generator pt extended range up to 2 TeV (needs a Flat DoubleElectron sample)
 - number of primary vertices

The plots are made separately for barrel and endcap, and for the full workingpoint
and each of its individual cuts. The "tag" argument is specifies the workingpoints
for which the efficiency has to be drawn (see the defined workingpoint sets in common.py)
The script is run as follows:

```
   ./drawEfficiency.py --tag=training106
```

Kinematic reweighting is applied in the script.
The output of the scripts is plots with names like:
```
  figures/efficiencies/WORKINGPOINTSET/plot_eff_barrel_pt.png
  figures/efficiencies/WORKINGPOINTSET/plot_eff_etaSC.png
  figures/efficiencies/WORKINGPOINTSET/plot_eff_barrel_nPV.pn
```

A couple of caveats regarding the cuts. The missing hits cuts are added
by hand, one needs to review and adjust them as needed.


## 9. 
Tune several cuts by manually. These include the expected missing
inner hits cuts and the impact parameter cuts. All of this is usually 
chosen to be fairly loose. Impact parameter cuts can then be changed
by individual analyses if needed.
   In the 2017, perhaps we should not change the d0 and dz cuts
from the 2016 values unless there is a reason to do it.
   As far as the expected missing inner hits, we should review the
cuts and see if the change is necessary with respect to the 2016 values
because we are now running with a new pixel detector with more layers.

   A discussion and evolution of the impact parameter cuts choice
in 2016 can be found in these presentations:
https://indico.cern.ch/event/545076/contributions/2212238/attachments/1295178/1930742/talk_electron_ID_spring16.pdf
https://indico.cern.ch/event/482675/contributions/2221671/attachments/1300610/1941558/talk_electron_ID_spring16_update.pdf
https://indico.cern.ch/event/482677/contributions/2259342/attachments/1316731/1972911/talk_electron_ID_spring16_update.pdf

 
## 10. 
The H/E cut tuned as a flat cut in this proceedure may need to be
reviewed, and scaling with rho and pt introduced along the lines discussed
in 
https://indico.cern.ch/event/662749/contributions/2763092/attachments/1545209/2425054/talk_electron_ID_2017.pdf
