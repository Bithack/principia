import fnmatch
import os
import sys

root_path = './'
base_output_dir = '../../data-mobile/sfx/'

exceptions = []

for root, dirs, files in os.walk(root_path):
    i = 0
    matching_files = fnmatch.filter(files, '*.wav')
    matching_len = len(matching_files)
    for filename in matching_files:
        i = i+1
        dir_arr = root.split('/')
        del dir_arr[0]
        output_dir = os.path.join(base_output_dir, '/'.join(dir_arr))

        last_dir = dir_arr[len(dir_arr)-1]

        if last_dir in ('ui', 'symbols'):
            continue

        if filename in exceptions:
            continue

        os.system('ffmpeg -i {0} -ac 1 {1}{0}'.format(filename, base_output_dir))

        #in_path = os.path.join(root, filename)

        #if last_dir == 'menu':
            #print 'flip png'
            #os.system('convert -flip {0} {1}'.format(in_path, tmp_file.format(filename)))
            #in_path = tmp_file.format(filename)
#
        #os.system('./bin/etcpack {0} {1} -c etc1 -s {2} -f RGB -ext PNG 1>/dev/null'.format(in_path, output_dir, speed))
