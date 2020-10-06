
# %%
# Import the os module, for the os.walk function
import os
import xml.etree.ElementTree as ET
 
# Set the directory you want to start from
# github:
# https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/ANN/Autoassociator/Autoassociator.ikc

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
        # short_descr = 
        long_descr = [elem.text for elem in root.iter('description')] 
        inps = [elem.attrib for elem in root.iter(tag='input')]
        outs = [elem.attrib for elem in root.iter(tag='output')]
        params = [elem.attrib for elem in root.iter(tag='parameter')]
        print(long_descr)
        retval.append((item[1][:-4], inps, outs, params, long_descr))
        
    return retval

def create_md_str(io_param_lst):
    retval_md = list()
    github_str = 'https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/'
    md_module_lst_str = '#Modules\n'
    for item in io_params_lst:
        name  = item[0]
        inps = item[1]
        outs = item[2]
        params = item[3]
        long_descr = item[4]
        short_descr = '<short descr placeholder>'

        #print(long_descr)
        #print()
        md_str = ''
        # heading: add name
        md_str += '#' + name + '\n'
        # subh: brief description
        md_str += '##Short description\n' + short_descr 
        # subh: inputs
        md_str += '\n##Inputs\n'
        md_str += 'Name|Description\n'
        md_str += '----|-----------\n'
        for inp in inps:
            md_str += inp['name'] + '|' + inp['description'] + '\n'
        # subh: outputs
        md_str += '\n##Outputs\n'
        md_str += 'Name|Description\n'
        md_str += '----|-----------\n'
        for out in outs:
            descr = ''
            if 'description' in out.keys():
                descr = out['description']
            md_str += out['name'] + '|' + descr + '\n'
        
        # subh: params
        md_str += '\n##Parameters\n'
        md_str += 'Name|Description|Type|Default value\n'
        md_str += '----|-----------|----|-------------\n'
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
            md_str += prm['name'] + '|' + descr + '|'
            md_str += type_str + '|' + deflt + '\n'
        
        # long description
        md_str += '##Long description\n' + long_descr 
        
        # add link to github
        # TODO

        # add module list, TODO sort list, 
        md_module_lst_str += '* [' + name + '](' + github_str + ') : ' + short_descr + '\n'

        retval_md.append(md_str)
    return retval_md, md_module_lst_str

# %% str tst
rootDir = '/Users/trond/code/ikaros/Source/Modules/'
ikc_list = get_ikc_list(rootDir)
io_params_lst = extract_io_params(ikc_list)
md_lst, module_lst = create_md_str(io_params_lst)
print(io_params_lst[:1])
#print (module_lst)
# %%
