#!/usr/bin/python

from wikitools import wiki
from wikitools import page
from wikitools import api
import sys
import re

import pprint # debug

pp = pprint.PrettyPrinter(indent=2)

insert = True
delete = False
print_object_list = True

object_path = '/home/pajlada/Principia/obj'

if insert or delete:
    site = wiki.Wiki('http://wiki.principiagame.com/api.php')
    if site.login('pajbot', 'xPalfk3Ma2', True):
        print 'Logged in as {0}.'.format(site.username)
    else:
        print 'unable to log in'
        sys.exit()

fh = open(object_path, 'r')
object_file_string = fh.read()
fh.close()

categories = {}
categories_tuple = ('Basic', 'Mechanics', 'Robotics', 'Electronics', 'Signaling', 'Misc', 'Game')
for category_name in categories_tuple:
    categories[category_name] = []

for obj in object_file_string.split('====='):
    g_id, name, description, category_name, layer, sublayer, edevice = obj.split(':::::')
    g_id = int(g_id)
    description = description.replace('\n', '<br />')
    is_edevice = False if edevice == '0' else True
    edevice = edevice[1:] # remove the 0 or 1 which specifies if it's an edevice or not
    s_in_array = []
    s_out_array = []

    if len(edevice):
        s_in, s_out = edevice.split('|')

        if len(s_in):
            s_in_data = s_in.split(':')
            s_in_array = [0] * len(s_in_data)
            for data in s_in_data:
                socket_index, cable_type, socket_tag, socket_description = data.split(';;')

                socket_index = int(socket_index)
                cable_type = int(cable_type)
                socket_tag = int(socket_tag)
                value = {'cable_type':cable_type, 'tag':socket_tag, 'description':'' if socket_description == '0' else socket_description}
                s_in_array[socket_index] = value

        if len(s_out):
            s_out_data = s_out.split(':')
            s_out_array = [0] * len(s_out_data)
            for data in s_out_data:
                socket_index, cable_type, socket_tag, socket_description = data.split(';;')

                socket_index = int(socket_index)
                cable_type = int(cable_type)
                socket_tag = int(socket_tag)
                value = {'cable_type':cable_type, 'tag':socket_tag, 'description':'' if socket_description == '0' else socket_description}
                s_out_array[socket_index] = value

    obj_value = {'g_id':g_id, 'name':name, 'description':description, 'layer':layer, 'sublayer':sublayer, 'is_edevice':is_edevice, 's_in':s_in_array, 's_out':s_out_array}

    categories.get(category_name).append(obj_value)

#pp.pprint(categories)

for category_name in categories_tuple:
    for obj in categories[category_name]:
        s = ''
        s_in = ''
        s_out = ''

        if len(obj['s_in']):
            for i, s_obj in enumerate(obj['s_in']):
                s_in += \
                '{{{{ socket\n' \
                '| type = IN\n' \
                '| index = {index}\n' \
                '| description = {self[description]}\n' \
                '| cable_type = {self[cable_type]}\n' \
                '| tag = {self[tag]}\n' \
                '}}}}\n'.format(index=i, self=s_obj)

        if len(obj['s_out']):
            for i, s_obj in enumerate(obj['s_out']):
                s_out += \
                '{{{{ socket\n' \
                '| type = OUT\n' \
                '| index = {index}\n' \
                '| description = {self[description]}\n' \
                '| cable_type = {self[cable_type]}\n' \
                '| tag = {self[tag]}\n' \
                '}}}}\n'.format(index=i, self=s_obj)

        s += \
            '{{{{ Object\n' \
            '| title = {self[name]}\n' \
            '| category = {category_name}\n' \
            '| description = {self[description]}\n' \
            '| g_id = {self[g_id]}\n' \
            '| is_edevice = {self[is_edevice]}\n' \
            '| s_in = {s_in}\n' \
            '| s_out = {s_out}\n' \
            '| layer = {self[layer]}\n' \
            '| sublayer = {self[sublayer]}\n' \
            '}}}}\n'.format(self=obj, s_in=s_in, s_out=s_out, category_name=category_name)

        s += '[[Category:Objects]] [[Category:{0}]]\n== User Information =='.format(category_name)

        if delete:
            x = page.Page(site, title=obj['name'])
            try:
                x.delete()
                print 'Deleted {0}'.format(obj['name'])
            except page.NoPage:
                print '{0} already deleted.'.format(obj['name'])

        if insert:
            obj_page = page.Page(site, title=obj['name'], sectionnumber=0)
            try:
                obj_page.edit(text=s, section=0, createonly=True)
                print 'Modified {0}'.format(obj['name'])
            except api.APIError:
                print '{0} already exists.'.format(obj['name'])

if print_object_list:
    s = ''
    for category_name in ('Basic', 'Mechanics', 'Robotics', 'Electronics', 'Signaling', 'Misc', 'Game'):
        s += "\n\n=== {0} ===\n".format(category_name)

        for obj in categories[category_name]:
            s += "* [[{0}]]\n".format(obj['name'])

    print s
