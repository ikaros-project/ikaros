"""Synthetic checks for composable spatial processing."""
from pathlib import Path
import sys,math
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'PatchCodebook/tests'))
from test_native_patches import run_case,close

def main():
    a,_=run_case('SensoryMismatch',dict(fields=1,pixels=2,alpha=.5),{'INPUT':[[1,0]],'PREDICTION':[[.5,.5]]},ticks=2);close(a[0],[.25]);close(a[1],[.3125])
    a,_=run_case('CategoryLocationBinding',dict(fields=1,categories=2,alpha=.5,support_gain=1,mismatch_gain=1,category_gain=0),{'INPUT':[[1,.5]],'SUPPORT':[1],'MISMATCH':[0],'CATEGORY':[0,0]});close(a[-1],[.5,.25])
    a,_=run_case('SpatialCompetition',dict(fields=2,categories=2,alpha=.5,category_gain=1),{'INPUT':[[1,0],[0,1]],'CATEGORY':[1,0]});close(a[-1],[.5,.25])
    a,_=run_case('SpatialCategoryPooling',dict(fields=2,categories=2),{'INPUT':[[1,0],[0,1]],'ATTENTION':[1,3]});close(a[-1],[.25,.75])
    a,_=run_case('SpatialGain',dict(height=1,width=3,fields=1,field_width=1,origin=1,sigma=1,alpha=1),{'INPUT':[[1,1,1]],'ATTENTION':[1]});v=(1+math.exp(-.5))/2;close(a[-1],[v,1,v])
    ref=[[[.2,.8],[.2,.8]],[[.8,.2],[.8,.2]]];blank=[[[.5,.5],[.5,.5]],[[.5,.5],[.5,.5]]]
    a,_=run_case('FeedbackAlignment',dict(fields=2,categories=2,references=2,channels=2,patch_rows=2,patch_columns=2),{'SLOTS':[[0,0],[1,1]],'ATTENTION':[1,0],'CATEGORY':[1,0]},{'BANK':[[ref,ref],[ref,ref]],'BLANK':blank})
    close(a[-1],[.2,.8,.2,.8,.8,.2,.8,.2,.8,.5,.8,.5,.2,.5,.2,.5])
    a,_=run_case('FieldReconstruction',dict(height=1,width=3,field_width=2,padding=1),{'INPUT':[[[0,1]],[[1,2]],[[2,3]],[[3,0]]]});close(a[-1],[1,2,3])
    print('PASS: lagged support, local binding, spatial competition, pooling, spotlight, reference alignment and display reconstruction.')
if __name__=='__main__':main()
