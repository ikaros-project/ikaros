
# %%
# Import the os module, for the os.walk function
import os
import xml.etree.ElementTree as ET
import re
 
# Set the directory you want to start from
# github:
# https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/ANN/Autoassociator/Autoassociator.ikc
# %%
def writefile(fname, content, confirm=False):
    fileobj = open(fname, 'w') # open for writing
    written = fileobj.write(content)
    fileobj.close()
    if confirm:
        print('Wrote ' + str(written) + ' to ' + fname)


def get_ikc_list(rootDir):
    '''
    Return list of module class names and their directories
    '''
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
    '''
    Returns list of tuples containing
    0 class name
    1 inputs
    2 outputs
    3 parameters
    4 long description
    5 short description
    6 directory address (from module level)
    '''
    # concat dir and fname
    retval = list()
    # create regular expression to extract module dirs
    prestr = '(^.+/Modules/)'
    capturestr = '([A-Za-z_0-9/]+)'
    p = re.compile(prestr + capturestr)

    for item in fname_list:
        fname = item[0] + '/' + item[1]
        # TODO extract dir name stub for github
        m = p.match(item[0])
        
        dir_adr = m.group(2)
        #print(dir_adr)
        root = ET.parse(fname).getroot()

        short_descr =  [elem.attrib['description'] for elem in root.iter('class')]
        # print (short_descr)
        long_descr = [ elem.text for elem in root.iter('description')]
        inps = [elem.attrib for elem in root.iter(tag='input')]
        outs = [elem.attrib for elem in root.iter(tag='output')]
        params = [elem.attrib for elem in root.iter(tag='parameter')]
        retval.append((item[1][:-4], inps, outs, params, 
            long_descr, short_descr, dir_adr))
        
    return retval

def create_md_str(io_param_lst):
    '''
    returns:
    - a list of strings for md files for each class

    '''
    retval_md = list()
    blankspace = '\n<br><br>\n'
    # github_str = 'https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/'
    # md_module_lst_str = '# Modules\n\n'
    for item in io_params_lst:
        name  = item[0]
        inps = item[1]
        outs = item[2]
        params = item[3]
        long_descr = item[4]
        short_descr_lst = item[5]
        dir_adr = item[6]
        dir_adr_lst = dir_adr.split('/')

        #print(long_descr)
        #print()
        md_str = ''
        # heading: add name
        md_str += '# ' + name + '\n\n'
        # image: TODO
        
        # subh: brief description
        short_descr = ''
        if len(short_descr_lst) > 0:
            short_descr = short_descr_lst[0].capitalize()
        md_str += blankspace + '## Short description\n\n' + short_descr + '\n'
        
        # subh: inputs
        
        md_str += blankspace + '\n## Inputs\n\n'
        md_str += '|Name|Description|Optional|\n'
        md_str += '|:----|:-----------|:-------|\n'
        for inp in inps:
            optional_str = 'No'
            if 'optional' in inp.keys():
                optional_str = inp['optional'].capitalize()
            md_str += '|' + inp['name'] + '|' + \
                inp['description'] + '|' + optional_str + '|' + '\n'
        
        # subh: outputs
        md_str += blankspace + '\n## Outputs\n\n'
        md_str += '|Name|Description|\n'
        md_str += '|:----|:-----------|\n'
        for out in outs:
            descr = ''
            if 'description' in out.keys():
                descr = out['description'].capitalize()
            md_str += '|' + out['name'] + '|' + descr + '|' + '\n'
        
        # subh: params
        md_str += blankspace + '\n## Parameters\n\n'
        md_str += '|Name|Description|Type|Default value|\n'
        md_str += '|:----|:-----------|:----|:-------------|\n'
        for prm in params:
            descr = ''
            if 'description' in prm.keys():
                descr = prm['description'].capitalize()
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
            descr = long_descr[0].strip()
        md_str += blankspace + '## Long description\n' + descr
        
        # add link to github
        # TODO

        
        retval_md.append((dir_adr, md_str))
    return retval_md #, md_module_lst_str

def create_module_lst_str(io_param_lst):
    '''
    Returns:
        - a string with a list of all module classes with links to github
    '''
    
    github_str = 'https://github.com/ikaros-project/ikaros/blob/master/Source/Modules/'
    md_module_lst_str = '# Modules\n\n'
    module_lst = list()
    for item in io_params_lst:
        # add module list, TODO sort list, 
        name  = item[0]
        # inps = item[1]
        # outs = item[2]
        # params = item[3]
        # long_descr = item[4]
        short_descr_lst = item[5]
        dir_adr = item[6]
        dir_adr_lst = dir_adr.split('/')

        short_descr = ''
        if len(short_descr_lst) > 0:
            short_descr = short_descr_lst[0].capitalize()
        
        if not name in dir_adr:
            dir_adr += '/' + name
        module_lst.append('* [' + dir_adr + '](' + \
            github_str + dir_adr +'/) : ' + short_descr + '\n')
    module_lst.sort()
    #print(module_lst)
    md_module_lst_str += ''.join(module_lst) 
    return md_module_lst_str

# %% str tst
# TODO replace with query
rootDir = '/home/trondarild/ikaros/ikaros/Source/Modules/'
readme_name = 'ReadMe.md'
ikc_list = get_ikc_list(rootDir)
io_params_lst = extract_io_params(ikc_list)
md_lst = create_md_str(io_params_lst)
module_lst = create_module_lst_str(io_params_lst)
#print (module_lst[:4000])
for i in range(len(md_lst)):
    fname = rootDir + md_lst[i][0] + '/' + readme_name
    writefile(fname, md_lst[i][1])
    #print ('wrote: ' + fname)
#writefile('ReadMe.md', md_lst[0])
#writefile('ikaros_module_list.md', module_lst)


# %%
# regular expression test
# '/home/trondarild/ikaros/ikaros/Source/Modules/'
prestr = '(^.+Modules/)'
capturestr = '([A-Za-z/]+)'
poststr = '(/.+.ikc?)'
tststr = '/home/trondarild/ikaros/ikaros/Source/Modules/ANN/Autoassociator/Autoassociator.ikc'
tststr2 = '/home/trondarild/ikaros/ikaros/Source/Modules/ANN/Autoassociator'
#p = re.compile(prestr + capturestr + poststr)
p = re.compile(prestr + capturestr)
m = p.match(tststr2)
print(m.group(1))
print(m.group(2))
#print(m.group(3))
#%%
tststr2 = '/home/trondarild/ikaros/ikaros/Source/Modules/ANN/Autoassociator/ReadMe.md'
writefile(tststr2, 'hello')
# %%
# test print hierarchy
rootDir = '/home/trondarild/ikaros/ikaros/Source/Modules/'
ikc_list = get_ikc_list(rootDir)
io_params_lst = extract_io_params(ikc_list)
name_adr = [(item[6].split('/'), item[0]) for item in io_params_lst]
retval = ''
for item in name_adr:
    levels = item[0]
    levelstr = ''
    for level in levels:
        levelstr+='*'
        retval += levelstr + ' ' + level.pop
        
