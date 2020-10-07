
# %%
# Import the os module, for the os.walk function
import os
import xml.etree.ElementTree as ET
 
# Set the directory you want to start from
# github:
# https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/ANN/Autoassociator/Autoassociator.ikc
def writefile(fname, content):
    fileobj = open(fname, 'w') # open for writing
    fileobj.write(content)
    fileobj.close()

def get_ikc_list(rootDir):
    retval = list()
    for dirName, subdirList, fileList in os.walk(rootDir):
        # print('Found directory: %s' % dirName)
        for fname in fileList:
            if fname[-3:] == 'ikc':
                # print(dirName)
                # print('\t%s' % fname)
                # read ikc file to extract io and params
                retval.append((dirName, fname))
    return retval

def extract_io_params(fname_list):
    # concat dir and fname
    retval = list()
    for item in fname_list:
        fname = item[0] + '/' + item[1]
        # TODO extract dir name stub for github
        #print ()
        #print (fname)
    
        root = ET.parse(fname).getroot()

        short_descr =  [elem.attrib['description'] for elem in root.iter('class')]
        # print (short_descr)
        long_descr = [ elem.text for elem in root.iter('description')]
        inps = [elem.attrib for elem in root.iter(tag='input')]
        outs = [elem.attrib for elem in root.iter(tag='output')]
        params = [elem.attrib for elem in root.iter(tag='parameter')]
        retval.append((item[1][:-4], inps, outs, params, long_descr, short_descr))
        
    return retval

def create_md_str(io_param_lst):
    retval_md = list()
    blankspace = '\n<br><br>\n'
    github_str = 'https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/'
    md_module_lst_str = '# Modules\n\n'
    for item in io_params_lst:
        name  = item[0]
        inps = item[1]
        outs = item[2]
        params = item[3]
        long_descr = item[4]
        short_descr_lst = item[5]

        #print(long_descr)
        #print()
        md_str = ''
        # heading: add name
        md_str += '# ' + name + '\n\n'
        # image: TODO
        # subh: brief description
        short_descr = ''
        if len(short_descr_lst) > 0:
            short_descr = short_descr_lst[0]
        md_str += blankspace + '## Short description\n\n' + short_descr + '\n'
        # subh: inputs
        md_str += blankspace + '\n## Inputs\n\n'
        md_str += '|Name|Description|\n'
        md_str += '|:----|:-----------|\n'
        for inp in inps:
            md_str += '|' + inp['name'] + '|' + inp['description'] + '|' + '\n'
        # subh: outputs
        md_str += blankspace + '\n## Outputs\n\n'
        md_str += '|Name|Description|\n'
        md_str += '|:----|:-----------|\n'
        for out in outs:
            descr = ''
            if 'description' in out.keys():
                descr = out['description']
            md_str += '|' + out['name'] + '|' + descr + '|' + '\n'
        
        # subh: params
        md_str += blankspace + '\n## Parameters\n\n'
        md_str += '|Name|Description|Type|Default value|\n'
        md_str += '|:----|:-----------|:----|:-------------|\n'
        for prm in params:
            descr = ''
            if 'description' in prm.keys():
                descr = prm['description']
            deflt = ''
            if 'default' in prm.keys():
                deflt = prm['default']
            type_str = ''
            if 'type' in prm.keys():
                type_str = prm['type']
            md_str += '|' + prm['name'] + '|' + descr + '|'
            md_str += type_str + '|' + deflt + '|' + '\n'
        
        # long description
        descr = ''
        if(len(long_descr) > 0):
            descr = long_descr[0]
        md_str += blankspace + '## Long description\n' + descr
        
        # add link to github
        # TODO

        # add module list, TODO sort list, 
        md_module_lst_str += '* [' + name + '](' + github_str + ') : ' + short_descr + '\n'

        retval_md.append(md_str)
    return retval_md, md_module_lst_str

# %% str tst
# TODO replace with query
rootDir = '/home/trondarild/ikaros/ikaros/Source/Modules/'

ikc_list = get_ikc_list(rootDir)
io_params_lst = extract_io_params(ikc_list)
md_lst, module_lst = create_md_str(io_params_lst)
# print(md_lst[0])
writefile('ReadMe.md', md_lst[0])
writefile('mod_list.md', module_lst)
# %%
