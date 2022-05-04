import lxml
import sys
import os
from io import BytesIO
from io import StringIO 
import urllib.request
from lxml import etree
import re

""""

Get rewrite rule from MPD formatted on the CMAF ingest structure 

Accorind to DASH-IF

Manifest with minimumUpdatePeriod set to a large value, this way
sender need not continuously update the MPD.

Availability start time should be 1970-01-01T00:00:00Z

each AdaptationSet (or otherwise each individual Representation) shall contain a SegmentTemplate element

SegmentTemplate@initiatization shall contain the substring $RepresentationID$
SegmentTempate@media shall contain the substring $RepresentationID$
and also contain the substring $Number$ or $Time$ (not both).

@media and @intialization are identical for each SegmentTemplate Element in the MPD

Representation@bandwidth shall be set to a correct value.
BaseURL element shall be absent
The manifest contains only a single period 
Period@start is set to PT0S

Each DASH Representation represents a CMAF track
Each DASH AdaptationSet represents a CMAF SwitchingSet

This way each segment can be explicitly mapped to the CMAF track and/or SwitchingSet

This program maps MPD to rewrite rules using Streams() keyword 
This script is tested only limted way

""""

def open_mpd(or_man_url, is_remote=False):
    
    parser = etree.XMLParser(remove_comments=True)
    tree, root = ([],[])
    
    if is_remote:
        if or_man_url[0:5] == 'https' :
            ctx = ssl._create_unverified_context()
            response = urllib.request.urlopen(
                or_man_url,
                context=ctx
            )
            mpd = response.read()
            tree = etree.parse(
                BytesIO(mpd),
                parser
            )
            root = tree.getroot()
            return (tree, root, parser)
        else:
            response = \
            urllib.request.urlopen(or_man_url)
            mpd = response.read()
            tree = etree.parse(
                BytesIO(mpd), 
                parser
            )
            root = tree.getroot()
            return (tree, root, parser)
    else:
        tree = etree.parse(
            or_man_url,
            parser
        )
        root = tree.getroot()
        return (tree,root,parser)

in_file="in.mpd"

if(len(sys.argv) > 1):
    in_file = sys.argv[1]

(tree,root,parser) = open_mpd(in_file) 

representation_id = []
seg_temp_media = []
seg_temp_init = []

for period in root:
    
    for a in period:
        
        if etree.QName(a.tag).localname == "AdaptationSet":

            for z in a:

                if etree.QName(z.tag).localname == \
                    "SegmentTemplate":
                    
                    seg_temp_init.append(str(z.get("initialization")))
                    seg_temp_media.append(str(z.get("media"))) 
                   
                if etree.QName(z.tag).localname == \
                    "Representation":
                    representation_id.append(str(z.get("id")))
                    
                    for y in z:
                        if etree.QName(y.tag).localname == "SegmentTemplate":
                            seg_temp_init.append(str(y.get("initialization")))
                            seg_temp_media.append(str(y.get("media"))) 

is_ok = 1

if len(seg_temp_media) == 0 or \
    len(seg_temp_init) == 0:
    print("no segment template found," + \
    "mandated for CMAF ingest MPD")
    is_ok = 0
else:
    rep_i = seg_temp_media[0].find("$RepresentationID$")
    rep_j = seg_temp_init[0].find("$RepresentationID$")
    
    if rep_i == -1 or rep_j == -1:
        print("segment template @media and @initialization shall"\
        "contain the substring $RepresentationID$, but not found!" ) 
        is_ok = 0
    for seg_temp in seg_temp_media: 
        if seg_temp != seg_temp_media[0]: 
            print("no segment template @media is not identical in the MPD")
            is_ok = 0
    for seg_init in seg_temp_init: 
        if seg_init != seg_temp_init[0]: 
            print("no segment template @initialization is not identical for in the MPD")
            is_ok = 0                           
before_init =''
after_init =''
before_media =''
after_media =''

if is_ok: 
    print("SegmentTemplate usage is"\
    " correct according to CMAF ingest defined in DASH-IF \n")
    ## get the part before and after the $Representation$ string 
    if rep_i != 0:
        before_media = \
        seg_temp_media[0][0:rep_i]
        

    if rep_i + 18 != len(seg_temp_media[0]):
        after_media = seg_temp_media[0][rep_i+18:]

    if rep_j != 0:
        before_init = seg_temp_init[0][0:rep_j]
        before_init = re.escape(before_init)
    
    if rep_j + 18 != len(seg_temp_init[0]):
        after_init = seg_temp_init[0][rep_j+18:]
        after_init = re.escape(after_init)

    if len(before_init):
        before_init = '(' + before_init + ')'
    
    if len(after_init):
        after_init = '(' + after_init + ')'
    
    print("RewriteCond \%\{REQUEST_METHOD\} ^(PUT|POST)")

    print("## general rewrite rule for init segments (loose, use carefully)")
    
    if len(before_init):
        print( 'RewriteRule "^(.*\.isml)/' + \
            before_init + '(.*)' + \
            after_init + '$" "$1/Streams($3)" [PT,L]')
    else:
        print( 'RewriteRule "^(.*\.isml)/' + \
            before_init + '(.*)' + \
            after_init + '$" "$1/Streams($2)" [PT,L]')

    print("## rewrite for init segments exact")
    
    for rep in representation_id:
        
        rep_esc = re.escape(rep)

        if len(before_init):
            print( 'RewriteRule "^(.*\.isml)/' + \
                before_init + '(' + rep_esc +')' + \
                after_init + '$" "$1/Streams($3)" [PT,L]')
        else:
            print( 'RewriteRule "^(.*\.isml)/' + \
                before_init + '(' + rep_esc +')' + \
                after_init + '$" "$1/Streams($2)" [PT,L]')
    
    if len(before_media):
        
        n_i = before_media.find("$Number$")
        t_i = before_media.find("$Time$")
        
        if n_i != -1:
            before_media = \
            re.escape(before_media[0:n_i]) +\
            '\d+' + \
            re.escape(before_media[n_i + 8:])
        
        if t_i != -1:
            before_media = \
            re.escape(before_media[0:t_i]) + \
            '\d+' + \
            re.escape(before_media[t_i + 6:])
        
        before_media = '(' + before_media + ')'
       
    if len(after_media):
        
        n_i = after_media.find("$Number$")
        t_i = after_media.find("$Time$")

        if n_i != -1:
            after_media = \
            re.escape(after_media[0:n_i]) + \
            '\d+' + \
            re.escape(after_media[n_i + 8:])
        
        if t_i != -1:
            after = \
            after[0:t_i] + \
            '\d+' + \
            after_media[t_i + 6:]
        
        after_media = '(' + after_media + ')'

    print("## rewrite for media segments loose (use carefully)")
    
    rep_esc = re.escape(rep)

    if len(before_media):
        print( 
            'RewriteRule "^(.*\.isml)/' + \
            before_media + '(.*)' + \
            after_media + '$" "$1/Streams($3)" [PT,L]'
        )
    else:
        print( 
            'RewriteRule "^(.*\.isml)/' + \
            '(.*)' + \
            after_media + '$" "$1/Streams($2)" [PT,L]'
        )

    print("## rewrite for media segment exact")
    
    for rep in representation_id:
        
        rep_esc = re.escape(rep)

        if len(before_media):
            print( 'RewriteRule "^(.*\.isml)/' + \
                before_media + '(' + rep_esc +')' + \
                after_media + '$" "$1/Streams($3)" [PT,L]'
            )
        else:
            print( 'RewriteRule "^(.*\.isml)/' + \
                '(' + rep_esc + ')' + \
                after_media + '$" "$1/Streams($2)" [PT,L]'
            )
    
    print('RewriteRule "^(/test/.*\.isml)/(.*\.mpd\|.*\.m3u8\)$" "/nothing" [R=204,L]') 
    print('RewriteCond %{REQUEST_METHOD} DELETE') 
    print('RewriteRule "^(/test/.*\.isml)/.*$" "/nothing" [R=204,L]')