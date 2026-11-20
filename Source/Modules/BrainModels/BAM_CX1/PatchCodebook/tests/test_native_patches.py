"""Synthetic numerical checks for native patch and projection modules."""
from pathlib import Path
import csv,json,math,os,subprocess,tempfile,xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[6]
def shape(a):return [len(a)]+shape(a[0]) if isinstance(a,list) else []
def run_case(cls,parameters,inputs,states=None,ticks=1,output_states=None):
    with tempfile.TemporaryDirectory() as tmp:
        tmp=Path(tmp);g=ET.Element('group',name='Synthetic',agent='native-component-verification',tick_duration='1')
        ET.SubElement(g,'module',{'class':cls,'name':'Model','_x':'300','_y':'80',**{k:str(v) for k,v in parameters.items()}})
        for i,(port,value) in enumerate(inputs.items()):
            ET.SubElement(g,'module',{'class':'Constant','name':port,'data':json.dumps(value),'_x':'30','_y':str(80+150*i)})
            ET.SubElement(g,'connection',source=port+'.OUTPUT',target='Model.'+port,delay='0')
        ET.SubElement(g,'module',{'class':'OutputFile','name':'Record','filename':'values.csv','format':'csv','timestamp':'none','number_format':'full','_x':'600','_y':'80'})
        ET.SubElement(g,'connection',source='Model.OUTPUT',target='Record.INPUT',delay='0',label='value')
        ET.ElementTree(g).write(tmp/'model.ikg')
        cmd=[str(ROOT/'Bin/ikaros'),'-b','-s',str(ticks),'-u',str(tmp),'-W',str(tmp/'saved.json')]
        if states or output_states:
            states = states or {}
            items={'Synthetic.Model.'+k:dict(kind='state',type='matrix',shape=shape(v),value=v) for k,v in states.items()}
            items.update({'Synthetic.Model.'+k:dict(kind='output',type='matrix',shape=shape(v),value=v) for k,v in (output_states or {}).items()})
            (tmp/'state.json').write_text(json.dumps(dict(format='ikaros-state-v1',scope='',items=items)))
            cmd+=['-L',str(tmp/'state.json')]
        cmd.append(str(tmp/'model.ikg'))
        env=os.environ.copy();env.update(HTTPS_PROXY='http://127.0.0.1:9',https_proxy='http://127.0.0.1:9')
        p=subprocess.run(cmd,capture_output=True,text=True,env=env)
        if p.returncode or not (tmp/'values.csv').exists():raise AssertionError(p.stdout+p.stderr)
        with (tmp/'values.csv').open() as f:rows=list(csv.reader(f))
        return [[float(x) for x in r] for r in rows[1:]],json.loads((tmp/'saved.json').read_text())
def close(actual,expected,tol=2e-6):
    assert len(actual)==len(expected),(actual,expected)
    assert all(abs(a-b)<=tol for a,b in zip(actual,expected)),(actual,expected)
def main():
    a,_=run_case('FieldExtraction',dict(height=1,width=3,field_width=2,padding=1),{'INPUT':[[1,2,3]]});close(a[-1],[0,1,1,2,2,3,3,0])
    a,_=run_case('BatchedProjection',dict(rows=2,input_size=2,output_size=4,softmax_channels=2),{'INPUT':[[1,0],[0,1]]},{'WEIGHTS':[[1,0,0,1],[0,1,1,0]],'BIAS':[0,0,0,0]})
    high=math.exp(1)/(1+math.exp(1));low=1-high;close(a[-1],[high,low,low,high,low,high,high,low])
    a,s=run_case('PatchCodebook',dict(fields=1,height=1,width=2,kernel_size=1,code_count=2,temperature=0,train='true',learning_rate=.5),{'INPUT':[[[0.2,0.8]]]},{'PROTOTYPES':[[0],[1]],'USAGE':[0,0]})
    close(a[-1],[1,0,0,1]);close([x[0] for x in s['items']['Synthetic.Model.PROTOTYPES']['value']],[.1,.9])
    a,_=run_case('PatchCodebook',dict(fields=1,height=1,width=1,kernel_size=1,code_count=2,temperature=.25),{'INPUT':[[[.25]]]},{'PROTOTYPES':[[0],[1]],'USAGE':[0,0]});close(a[-1],[1/(1+math.exp(-2)),1/(1+math.exp(2))])
    a,_=run_case('PatchReconstruction',dict(fields=1,height=2,width=3,kernel_size=2,code_count=1,clip='false'),{'INPUT':[[[[1,1]]]]},{'WEIGHTS':[[1,2,3,4]]});close(a[-1],[1,1.5,2,3,3.5,4])
    print('PASS: extraction, categorical projection, competitive learning, soft coding, overlap reconstruction and state persistence.')
if __name__=='__main__':main()
