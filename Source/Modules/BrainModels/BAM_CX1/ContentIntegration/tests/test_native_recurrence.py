"""A loaded population survives warmup and then follows a one-tick recurrence."""
from pathlib import Path
import csv,json,os,subprocess,tempfile,xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[6]
with tempfile.TemporaryDirectory() as folder:
    folder=Path(folder)
    (folder/'hold.csv').write_text('1\n0\n')
    group=ET.Element('group',name='Synthetic',agent='native-component-verification')
    for name,cls,x,y,params in [
        ('Sensory','Constant',50,50,dict(data='[1]')),
        ('Warmup','InputFile',50,250,dict(filename='hold.csv',header='false',send_end_of_file='false')),
        ('Content','ContentIntegration',400,50,dict(rows='1',features='1',alpha='.2',fraction='.5')),
        ('Record','OutputFile',750,50,dict(filename='values.csv',format='csv',timestamp='none',number_format='full'))]:
        ET.SubElement(group,'module',{'name':name,'class':cls,'_x':str(x),'_y':str(y),**params})
    for source,target,delay in [('Sensory.OUTPUT','Content.INPUT',0),('Content.OUTPUT','Content.PREDICTION',1),('Warmup.OUTPUT','Content.HOLD',0),('Content.OUTPUT','Record.INPUT',0)]:
        ET.SubElement(group,'connection',source=source,target=target,delay=str(delay))
    ET.ElementTree(group).write(folder/'model.ikg')
    (folder/'state.json').write_text(json.dumps(dict(format='ikaros-state-v1',scope='',items={'Synthetic.Content.OUTPUT':dict(kind='output',type='matrix',shape=[1,1],value=[[.25]])})))
    env=os.environ.copy();env.update(HTTPS_PROXY='http://127.0.0.1:9',https_proxy='http://127.0.0.1:9')
    p=subprocess.run([str(ROOT/'Bin/ikaros'),'-b','-s','4','-u',str(folder),'-L',str(folder/'state.json'),str(folder/'model.ikg')],capture_output=True,text=True,env=env)
    assert p.returncode==0,p.stdout+p.stderr
    with (folder/'values.csv').open() as f:actual=[float(row[0]) for row in list(csv.reader(f))[1:]]
    expected=[.25,.325,.3925,.45325]
    assert len(actual)==len(expected) and max(abs(a-b) for a,b in zip(actual,expected))<1e-6,(actual,expected)
print('PASS: loaded state, explicit warmup and delayed recurrent trajectory.')
