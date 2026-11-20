"""Synthetic category competition and feedback checks."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'PatchCodebook/tests'))
from test_native_patches import run_case,close

def main():
    a,_=run_case('CategoryEvidence',dict(rows=1,input_size=2,categories=2,units_per_category=2),{'INPUT':[[1,2]]},{'WEIGHTS':[[0,0,0,0],[0,0,0,0]],'BIAS':[0,0,0,0]});close(a[-1],[.5,.5])
    a,s=run_case('CategoryEvidence',dict(rows=1,input_size=2,categories=2,units_per_category=1,train='true',label_smoothing=0,weight_decay=0),{'INPUT':[[1,2]],'LABELS':[1]},{'WEIGHTS':[[0,0],[0,0]],'BIAS':[0,0]})
    w=s['items']['Synthetic.Model.WEIGHTS']['value'];assert w[0][0]<0<w[0][1] and w[1][0]<0<w[1][1]
    a,_=run_case('CategoryCompetition',dict(rows=1,categories=2,alpha=.5,inhibition=1),{'INPUT':[[1,.5]]},ticks=2);close(a[0],[.5,.25]);close(a[1],[.625,.125])
    a,_=run_case('ReferenceSelection',dict(rows=1,categories=2,references=2,latent_size=2),{'INPUT':[[.8,0]]},{'BANK':[[[0,0],[1,0]],[[0,2],[2,0]]]});close(a[-1],[1,0,2,0])
    a,_=run_case('CategoryPrediction',dict(rows=1,categories=2,features=2),{'INPUT':[[[1,0],[0,1]]],'ACTIVITY':[[1,3]]});close(a[-1],[.25,.75])
    a,_=run_case('ContentIntegration',dict(rows=1,features=2,alpha=.5,fraction=.5),{'INPUT':[[1,0]],'PREDICTION':[[0,1]]},ticks=2);close(a[0],[.25,.25]);close(a[1],[.375,.375])
    a,_=run_case('ContentIntegration',dict(rows=1,features=2,alpha=.5,fraction=1),{'INPUT':[[1,0]],'PREDICTION':[[0,1]],'RESET':[1]});close(a[-1],[1,0])
    a,s=run_case('CategoryCompetition',dict(rows=1,categories=2),{'INPUT':[[1,0]],'HOLD':[1],'RESET':[1]},output_states={'OUTPUT':[[.2,.4]]},ticks=2);close(a[-1],[.2,.4])
    a,_=run_case('ContentIntegration',dict(rows=1,features=2),{'INPUT':[[1,0]],'PREDICTION':[[0,1]],'HOLD':[1],'RESET':[1]},output_states={'OUTPUT':[[.3,.7]]},ticks=2);close(a[-1],[.3,.7])
    print('PASS: grouped evidence, local learning, synchronous competition, references, category prediction and integration/reset.')
if __name__=='__main__':main()
