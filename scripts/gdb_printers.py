import gdb.printing

# https://sourceware.org/gdb/onlinedocs/gdb/Pretty-Printing.html
# https://sourceware.org/gdb/onlinedocs/gdb/Writing-a-Pretty_002dPrinter.html

COORDINATE_PRECISION = 1e6
coord2float = lambda x: int(x) / COORDINATE_PRECISION
lonlat = lambda x: (coord2float(x['lon']['__value']), coord2float(x['lat']['__value']))

def call(this, method, *args):
    """Call this.method(args)"""
    if (str(this) == '<optimized out>'):
        raise BaseException('"this" is optimized out')
    command = '(*({})({})).{}({})'.format(this.type.target().pointer(), this.address, method, ','.join((str(x) for x in args)))
    return gdb.parse_and_eval(command)

def iterate(v):
    s, e = v['_M_impl']['_M_start'], v['_M_impl']['_M_finish']
    while s != e:
        yield s.dereference()
        s +=1

class CoordinatePrinter:
    """Print a CoordinatePrinter object."""
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return '{{{}, {}}}'.format(*lonlat(self.val))

class TurnInstructionPrinter:
    """Print a TurnInstruction object."""

    modifiers = {0:'UTurn', 1:'SharpRight', 2:'Right', 3:'SlightRight',
                 4:'Straight', 5:'SlightLeft', 6:'Left', 7:'SharpLeft'}
    types = {0:'Invalid', 1:'NewName', 2:'Continue', 3:'Turn', 4:'Merge', 5:'OnRamp',
             6:'OffRamp', 7:'Fork', 8:'EndOfRoad', 9:'Notification', 10:'EnterRoundabout',
             11:'EnterAndExitRoundabout', 12:'EnterRotary', 13:'EnterAndExitRotary',
             14:'EnterRoundaboutIntersection', 15:'EnterAndExitRoundaboutIntersection',
             16:'UseLane', 17:'NoTurn', 18:'Suppressed', 19:'EnterRoundaboutAtExit',
             20:'ExitRoundabout', 21:'EnterRotaryAtExit', 22:'ExitRotary',
             23:'EnterRoundaboutIntersectionAtExit', 24:'ExitRoundaboutIntersection',
             25:'StayOnRoundabout', 26:'Sliproad'}

    def __init__(self, val):
        self.val = val

    def to_string(self):
        t, m = int(self.val['type']), int(self.val['direction_modifier'])
        m = '%s (%d)' % (self.modifiers[m], m) if m in self.modifiers else str(m)
        t = '%s (%d)' % (self.types[t], t) if t in self.types else str(t)
        return '{{type = {}, direction_modifier = {}}}'.format(t, m)

class TurnLaneDataPrinter:
    """Print a TurnLaneData object."""

    mask = {0:'Empty', 1:'None', 2:'Straight', 4:'SharpLeft', 8:'Left', 16:'SlightLeft',
            32:'SlightRight', 64:'Right', 128:'SharpRight', 256:'UTurn', 512:'MergeToLeft',
            1024:'MergeToRight'}

    def __init__(self, val):
        self.val = val

    def to_string(self):
        tg = int(self.val['tag'])
        fr, to = int(self.val['from']), int(self.val['to'])
        return '{{tag = {}, from = {}, to = {}}}'.format(self.mask[tg] if tg in self.mask else tg, fr, to)

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('OSRM')
    pp.add_printer('TurnInstruction', '::TurnInstruction$', TurnInstructionPrinter)
    pp.add_printer('Coordinate', '::Coordinate$', CoordinatePrinter)
    pp.add_printer('TurnLaneData', '::TurnLaneData$', TurnLaneDataPrinter)
    return pp

## unregister OSRM pretty printer before (re)loading
gdb.pretty_printers = [x for x in gdb.pretty_printers if not isinstance(x, gdb.printing.RegexpCollectionPrettyPrinter) or x.name != 'OSRM']
gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printer())

import geojson
import os
import time
import tempfile
import urllib.parse
import webbrowser
import re

class GeojsonPrinter (gdb.Command):
    """Display features on geojson.io."""

    def __init__ (self):
        super (GeojsonPrinter, self).__init__ ('geojson', gdb.COMMAND_USER)
        self.to_geojson = {
            'osrm::engine::guidance::RouteSteps': self.RouteSteps,
            'std::vector<osrm::engine::guidance::RouteStep, std::allocator<osrm::engine::guidance::RouteStep> >': self.RouteSteps}

    @staticmethod
    def encodeURIComponent(s):
        return urllib.parse.quote(s.encode('utf-8'), safe='~()*!.\'')

    @staticmethod
    def RouteSteps(steps):
        k, road, result = 0, [], []
        for step in iterate(steps):
            maneuver, location = step['maneuver'], step['maneuver']['location']
            ll = lonlat(location)
            road.append(ll)

            properties= {field.name: str(step[field.name]) for field in step.type.fields() if str(step[field.name]) != '""'}
            properties.update({'maneuver.' + field.name: str(maneuver[field.name]) for field in maneuver.type.fields()})
            properties.update({'stroke': '#0000ff', 'stroke-opacity': 0.8, 'stroke-width': 15})
            result.append(geojson.Feature(geometry=geojson.LineString([ll, ll]), properties=properties))

        road = geojson.Feature(geometry=geojson.LineString(road), properties={'stroke': '#0000ff', 'stroke-opacity': 0.5, 'stroke-width':5})
        return [road, *result]

    def invoke (self, arg, from_tty):
        try:
            val = gdb.parse_and_eval(arg)
            features = self.to_geojson[str(val.type)](val)
            request = self.encodeURIComponent(str(geojson.FeatureCollection(features)))
            webbrowser.open('http://geojson.io/#data=data:application/json,' + request)
        except KeyError as e:
            print ('no GeoJSON printer for: ' + str(e))
        except gdb.error as e:
            print('error: ' % (e.args[0] if len(e.args)>0 else 'unspecified'))
            return

GeojsonPrinter()



class SVGPrinter (gdb.Command):
    """
    Generate SVG representation within HTML of edge-based graph in facade.
    SVG image contains:
    - thick lines with arrow heads are edge-based graph nodes with forward (green) and reverse (red) node IDs (large font)
    - segments weights are numbers (small font) in the middle of segments in forward (green) or reverse (red) direction
    - thin lines are edge-based graph edges in forward (green), backward (red) or both (yellow) directions with
      weights, edge-based graph node IDs (source, targte) and some algorithm-specific information
    - coordinates of segments end points (node-based graph nodes)
    """

    def __init__ (self):
        super (SVGPrinter, self).__init__ ('svg', gdb.COMMAND_USER)
        self.re_bbox = None
        self.to_svg = {
            'const osrm::engine::datafacade::ContiguousInternalMemoryDataFacade<osrm::engine::routing_algorithms::ch::Algorithm> &': self.Facade,
            'const osrm::engine::datafacade::ContiguousInternalMemoryDataFacade<osrm::engine::routing_algorithms::mld::Algorithm> &': self.Facade,
            'osrm::engine::routing_algorithms::Facade': self.Facade,
            'osrm::engine::DataFacade': self.Facade}


    @staticmethod
    def show_svg(svg, width, height):
        svg = """<!DOCTYPE HTML>
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
<head><meta http-equiv="content-type" content="application/xhtml+xml; charset=utf-8"/>
  <style>
    svg { background-color: beige; }
    .node { stroke: #000; stroke-width: 4; fill: none; marker-end: url(#forward) }
    .node.forward { stroke-width: 2; stroke: #080; font-family: sans; font-size: 42px }
    .node.reverse { stroke-width: 2; stroke: #f00; font-family: sans; font-size: 42px }
    .segment { marker-start: url(#osm-node); marker-end: url(#osm-node); }
    .segment.weight { font-family: sans; font-size:24px; text-anchor:middle; stroke-width: 1;  }
    .segment.weight.forward { stroke: #080; fill: #080; }
    .segment.weight.reverse { stroke: #f00; fill: #f00; }
    .edge { stroke: #00f; stroke-width: 2; fill: none; }
    .edge.forward { stroke: #0c0; stroke-width: 1; marker-end: url(#forward) }
    .edge.backward { stroke: #f00; stroke-width: 1; marker-start: url(#reverse) }
    .edge.both { stroke: #fc0; stroke-width: 1; marker-end: url(#forward); marker-start: url(#reverse) }
    .coordinates { font-size: 12px; fill: #333 }
  </style>
</head>
<svg viewBox="0 0 """ + str(width) + ' ' + str(height) + """"
     xmlns="http://www.w3.org/2000/svg"
     xmlns:xlink="http://www.w3.org/1999/xlink">
  <defs>
    <marker id="forward" markerWidth="10" markerHeight="10" refX="9" refY="3" orient="auto" markerUnits="strokeWidth">
      <path d="M0,0 C3,3 3,3 0,6 L9,3 z" fill="#000" />
    </marker>

    <marker id="reverse" markerWidth="10" markerHeight="10" refX="0" refY="3" orient="auto" markerUnits="strokeWidth">
      <path d="M9,0 C6,3 6,3 9,6 L0,3 z" fill="#000" />
    </marker>

    <marker id="osm-node" markerWidth="10" markerHeight="10" refX="5" refY="5" orient="auto" markerUnits="strokeWidth">
      <circle cx="5" cy="5" r="5"  fill="#000" />
    </marker>
  </defs>
""" + svg + '\n</svg></html>'
        fd, name = tempfile.mkstemp('.html')
        os.write(fd, svg.encode('utf-8'))
        os.close(fd)
        print ('Saved to ' + name)
        webbrowser.open('file://' + name)

    @staticmethod
    def getByGeometryId(facade, id, value):
        return call(facade, 'GetUncompressed' + ('Forward' if id['forward'] else 'Reverse') + value, id['id'])

    @staticmethod
    def getNodesInBoundingBox(facade, bbox):
        nodes, longitudes, latitudes = set(), set(), set()
        for node in range(call(facade, 'GetNumberOfNodes')):
            geometry = SVGPrinter.getByGeometryId(facade, call(facade, 'GetGeometryIndex', node), 'Geometry')
            node_longitudes, node_latitudes, in_bbox = set(), set(), False
            for nbg_node in iterate(geometry):
                lon, lat = lonlat(call(facade, 'GetCoordinateOfNode', nbg_node))
                node_longitudes.add(lon)
                node_latitudes.add(lat)
                in_bbox = in_bbox or bbox[0] <= lon and lon <= bbox[2] and bbox[1] <= lat and lat <= bbox[3]
            if in_bbox:
                nodes.add(node)
                longitudes.update(node_longitudes)
                latitudes.update(node_latitudes)
        return nodes, longitudes, latitudes

    @staticmethod
    def Facade(facade, width, height, arg):

        result = ''

        ## parse additional facade arguments
        re_float = '[-+]?[0-9]*\.?[0-9]+'
        bbox = re.search('(' + re_float + '),(' + re_float + ');(' + re_float + '),(' + re_float +')', arg)
        bbox = [float(x) for x in bbox.groups()] if bbox else [-180, -90, 180, 90]
        mld_level = re.search('L:([0-9]+)', arg)
        mld_level = int(mld_level.group(1)) if mld_level else 0

        marginx, marginy = 75, 75
        INVALID_SEGMENT_WEIGHT, MAX_SEGMENT_WEIGHT = gdb.parse_and_eval('INVALID_SEGMENT_WEIGHT'), gdb.parse_and_eval('INVALID_SEGMENT_WEIGHT')
        segment_weight = lambda x: str(x) + (' invalid' if x == INVALID_SEGMENT_WEIGHT else ' max' if x == MAX_SEGMENT_WEIGHT else '')

        if mld_level > 0:
            mld_facade = facade.cast(gdb.lookup_type('osrm::engine::datafacade::ContiguousInternalMemoryAlgorithmDataFacade<osrm::engine::routing_algorithms::mld::Algorithm>'))
            mld_partition = mld_facade['mld_partition']
            mld_levels = call(mld_partition, 'GetNumberOfLevels')
            if mld_level < mld_levels:
                sentinel_node = call(mld_partition['partition'], 'size') - 1 # GetSentinelNode
                number_of_cells = call(mld_partition, 'GetCell', mld_level, sentinel_node) # GetNumberOfCells
                result += "<defs>"
                for cell in range(number_of_cells):
                    result += """
<filter x="0" y="0" width="1" height="1" id="cellid-{}"><feFlood flood-color="hsl({}, 100%, 82%)"/><feComposite in="SourceGraphic"/></filter>""" \
                    .format(cell, int(256 * cell / number_of_cells))
                result += "\n</defs>"
            else:
                mld_level = 0
        else:
            mld_levels = 0

        ## get nodes
        nodes, longitudes, latitudes = SVGPrinter.getNodesInBoundingBox(facade, bbox)
        if len(nodes) == 0:
            return ''

        ## create transformations (lon,lat) -> (x,y)
        minx, miny, maxx, maxy = min(longitudes), min(latitudes), max(longitudes), max(latitudes)
        if abs(maxx - minx) < 1e-8:
            maxx += (maxy - miny) / 2
            minx -= (maxy - miny) / 2
        if abs(maxy - miny) < 1e-8:
            maxy += (maxx - minx) / 2
            miny -= (maxx - minx) / 2
        tx = lambda x: marginx + (x - minx) * (width - 2 * marginx) / (maxx - minx)
        ty = lambda y: marginy + (maxy - y) * (height - 2 * marginy) / (maxy - miny)
        t = lambda x: str(tx(x[0])) + ',' + str(ty(x[1]))

        print ('Graph has {} nodes and {} edges and {} nodes in the input bounding box {},{};{},{} -> {},{};{},{}'
               .format(call(facade, 'GetNumberOfNodes'), call(facade, 'GetNumberOfEdges'), len(nodes), *bbox, minx, miny, maxx, maxy))

        for node in nodes:
            geometry_id = call(facade, 'GetGeometryIndex', node)
            direction = 'forward' if geometry_id['forward'] else 'reverse'
            print (geometry_id, direction)
            geometry = SVGPrinter.getByGeometryId(facade, geometry_id, 'Geometry')
            weights = SVGPrinter.getByGeometryId(facade, geometry_id, 'Weights')



            ## add the edge-based node
            ref = 'n' + str(node)
            cell_background = ' filter="url(#cellid-{})"'.format(call(mld_partition, 'GetCell', mld_level, node)) if mld_level > 0 else ''
            result += '<path id="' + ref + '" class="node" d="M' \
                                 + ' L'.join([t(lonlat(call(facade, 'GetCoordinateOfNode', x))) for x in iterate(geometry)]) \
                                 + '" />'
            result += '<text' + cell_background + '><textPath class="node ' + direction + '" xlink:href="#' + ref \
                                 + '" startOffset="60%">' + str(node) + '</textPath></text>\n'

            ## add segments with weights
            geometry_first = geometry['_M_impl']['_M_start']
            for segment, weight in enumerate(iterate(weights)):
                ref = 's' + str(node) + '.' + str(segment)
                fr = lonlat(call(facade, 'GetCoordinateOfNode', geometry_first.dereference()))
                to = lonlat(call(facade, 'GetCoordinateOfNode', (geometry_first+1).dereference()))
                if fr == to:
                    ## node penalty on zero length segment (traffic light)
                    result += '<text class="segment weight ' + direction \
                           + '" x="' + str(tx(fr[0])) + '" y="' + str(ty(fr[1])) \
                           + '" font="Arial" font-size="32" rotate="0" text-anchor="middle" >' \
                           + '&#x1F6A6; ' + segment_weight(weight) + '</text>\n'
                else:
                    ## normal segment
                    result += '<path id="' + ref + '" class="segment" d="'  \
                           + 'M' + t(fr) + ' ' \
                           + 'L' + t(to) + '" />'\
                           + '<text class="segment weight ' + direction + '">'\
                           + '<textPath xlink:href="#' + ref + '" startOffset="50%">' \
                           + segment_weight(weight) + '</textPath></text>\n'
                geometry_first += 1

            ## add edge-based edges
            s0, s1 = geometry['_M_impl']['_M_start'].dereference(), (geometry['_M_impl']['_M_start'] + 1).dereference()
            for edge in []: # range(call(facade, 'BeginEdges', node), call(facade, 'EndEdges', node)): adjust to GetAdjacentEdgeRange
                target, edge_data = call(facade, 'GetTarget', edge), call(facade, 'GetEdgeData', edge)
                direction = 'both' if edge_data['forward'] and edge_data['backward'] else 'forward' if edge_data['forward'] else 'backward'
                target_geometry = SVGPrinter.getByGeometryId(facade, call(facade, 'GetGeometryIndex', target), 'Geometry')
                t0, t1 = target_geometry['_M_impl']['_M_start'].dereference(), (target_geometry['_M_impl']['_M_start'] + 1).dereference()

                ## the source point: the first node of the source node's first segment
                s0x, s0y = lonlat(call(facade, 'GetCoordinateOfNode', s0))

                ## the first control point: the node orthogonal to the first segment at the middle of the segment and offset distance length / 4
                s1x, s1y = lonlat(call(facade, 'GetCoordinateOfNode', s1))
                d0x, d0y = s1x - s0x, s1y - s0y
                c0x, c0y = s0x + d0x /2 - d0y /4, s0y + d0y / 2 + d0x /4

                ## the end point: middle of the first segment of the target node
                t0x, t0y = lonlat(call(facade, 'GetCoordinateOfNode', t0))
                t1x, t1y = lonlat(call(facade, 'GetCoordinateOfNode', t1))
                d1x, d1y = t1x - t0x, t1y - t0y
                e1x, e1y = t0x + d1x / 2, t0y + d1y / 2

                ## the second control point: the first node of the target's node first segment
                c1x, c1y = t0x, t0y

                ref = 'e' + str(edge)
                edge_arrow = ('↔' if edge_data['backward'] else '→') if edge_data['forward'] else ('←' if edge_data['backward'] else '?')
                text = str(node) + edge_arrow + str(target) + ' ' + str(edge_data['weight']) \
                       + (', shortcut' if 'shortcut' in set([x.name for x in edge_data.type.target().fields()]) and edge_data['shortcut'] else '')
                result += '<path id="' + ref + '" class="edge ' + direction + '" d="'  \
                                + 'M' + t((s0x, s0y)) + ' ' \
                                + 'C' + t((c0x, c0y)) + ' ' + t((c1x, c1y)) + ' ' + t((e1x, e1y)) + '" />'\
                                + '<text>'\
                                + '<textPath xlink:href="#' + ref + '" startOffset="' + ('60' if edge_data['forward'] else '40') + '%">' \
                                + text + '</textPath></text>\n'
        result += '\n\n'

        ## add longitudes and latitudes
        for lon in longitudes:
            result += '<text x="' + str(tx(lon)) + '" y="20" class="coordinates" text-anchor="middle">' + str(lon) + '</text>\n'
        for lat in latitudes:
            result += '<text x="20" y="' + str(ty(lat)) + '" transform="rotate(-90 20 ' \
                                + str(ty(lat)) + ')" class="coordinates" text-anchor="middle">' + str(lat) + '</text>\n'
        return result

    def invoke (self, arg, from_tty):
        try:
            argv = arg.split(' ')
            if len(argv) == 0 or len(argv[0]) == 0:
                print ('no argument specified\nsvg <varname> [BOUNDING BOX west,south;east,north] [SIZE width,height] [L:MLD level]')
                return
            val = gdb.parse_and_eval(argv[0])
            dims = re.search('([0-9]+)x([0-9]+)', arg)
            width, height = [int(x) for x in dims.groups()] if dims else (2100, 1600)
            type = val.type.target().unqualified() if val.type.code == gdb.TYPE_CODE_REF else val.type
            svg = self.to_svg[str(type)](val, width, height, arg)
            self.show_svg(svg, width, height)
        except KeyError as e:
            print ('no SVG printer for: ' + str(e))
        except gdb.error as e:
            print('error: ' % (e.args[0] if len(e.args)>0 else 'unspecified'))

SVGPrinter()
