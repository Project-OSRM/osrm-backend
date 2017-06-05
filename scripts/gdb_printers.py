import gdb.printing

# https://sourceware.org/gdb/onlinedocs/gdb/Pretty-Printing.html
# https://sourceware.org/gdb/onlinedocs/gdb/Writing-a-Pretty_002dPrinter.html

COORDINATE_PRECISION = 1e6
coord2float = lambda x: int(x) / COORDINATE_PRECISION
lonlat = lambda x: (coord2float(x['lon']['__value']), coord2float(x['lat']['__value']))

class CoordinatePrinter:
    """Print a CoordinatePrinter object."""
    def __init__(self, val):
        self.val = val

    def to_string(self):
        lon, lat = int(self.val['lon']['__value']), int(self.val['lat']['__value'])
        return '{{{}, {}}}'.format(float(lon) / COORDINATE_PRECISION, float(lat) / COORDINATE_PRECISION)

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

gdb.pretty_printers = []
gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printer())


import geojson
import os
import time
import tempfile
import urllib.parse
import webbrowser

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
        k = 0
        start, finish, road, steps = steps['_M_impl']['_M_start'], steps['_M_impl']['_M_finish'], [], []
        while start != finish:
            step = start.dereference()

            maneuver, location = step['maneuver'], step['maneuver']['location']
            ll = lonlat(location)
            road.append(ll)

            properties= {field.name: str(step[field.name]) for field in step.type.fields() if str(step[field.name]) != '""'}
            properties.update({'maneuver.' + field.name: str(maneuver[field.name]) for field in maneuver.type.fields()})
            properties.update({'stroke': '#0000ff', 'stroke-opacity': 0.8, 'stroke-width': 15})
            steps.append(geojson.Feature(geometry=geojson.LineString([ll, ll]), properties=properties))

            start += 1
        road = geojson.Feature(geometry=geojson.LineString(road), properties={'stroke': '#0000ff', 'stroke-opacity': 0.5, 'stroke-width':5})
        return [road, *steps]

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
    """Display features on geojson.io."""

    def __init__ (self):
        super (SVGPrinter, self).__init__ ('svg', gdb.COMMAND_USER)
        self.to_svg = {
            'const osrm::engine::datafacade::ContiguousInternalMemoryDataFacade<osrm::engine::routing_algorithms::ch::Algorithm> &': self.Facade,
            'const osrm::engine::datafacade::ContiguousInternalMemoryDataFacade<osrm::engine::routing_algorithms::corech::Algorithm> &': self.Facade,
            'const osrm::engine::datafacade::ContiguousInternalMemoryDataFacade<osrm::engine::routing_algorithms::mld::Algorithm> &': self.Facade}


    @staticmethod
    def show_svg(svg, width, height):
        svg = """<!DOCTYPE HTML>
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
<head><meta http-equiv="content-type" content="application/xhtml+xml; charset=utf-8"/>
  <style>
    svg { background-color: beige; }
    .node { stroke: #000; stroke-width: 4; fill: none; marker-end: url(#forward) }
    .node.forward { stroke-width: 2; stroke: #0c0; font-family: sans; font-size: 42px }
    .node.reverse { stroke-width: 2; stroke: #f00; font-family: sans; font-size: 42px }
    .segment { marker-start: url(#osm-node); marker-end: url(#osm-node); }
    .segment.weight { font-family: sans; font-size:24px; text-anchor:middle; stroke-width: 1;  }
    .segment.weight.forward { stroke: #0c0; fill: #0c0; }
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
    def Facade(facade, width, height):
        call = lambda val, method, *args: gdb.parse_and_eval("(*("+str(val.type.target().pointer())+")("+str(val.address)+"))." + method + "(" + ",".join((str(x) for x in args)) + ")")
        get_by_geometry_id = lambda id, m: call(facade, 'GetUncompressed' + ('Forward' if id['forward'] else 'Reverse') + m, id['id'])

        longitudes, latitudes = set(), set()
        for node in range(call(facade, 'GetNumberOfNodes')):
            geometry = get_by_geometry_id(call(facade, 'GetGeometryIndex', node), 'Geometry')
            geometry_first, geometry_last = geometry['_M_impl']['_M_start'], geometry['_M_impl']['_M_finish']
            while geometry_first != geometry_last:
                lon, lat = lonlat(call(facade, 'GetCoordinateOfNode', geometry_first.dereference()))
                longitudes.add(lon)
                latitudes.add(lat)
                geometry_first += 1
        minx, miny, maxx, maxy = min(longitudes), min(latitudes), max(longitudes), max(latitudes)
        if abs(maxx - minx) < 1e-8:
            maxx += (maxy - miny) / 2
            minx -= (maxy - miny) / 2
        if abs(maxy - miny) < 1e-8:
            maxy += (maxx - minx) / 2
            miny -= (maxx - minx) / 2
        tx = lambda x: 50 + (x - minx) * (width - 2 * 50) / (maxx - minx)
        ty = lambda y: 50 + (maxy - y) * (height - 2 * 50) / (maxy - miny)
        t = lambda x: str(tx(x[0])) + ',' + str(ty(x[1]))
        def it(v):
            s, e = v['_M_impl']['_M_start'], v['_M_impl']['_M_finish']
            while s != e:
                yield s.dereference()
                s +=1
        INVALID_SEGMENT_WEIGHT, MAX_SEGMENT_WEIGHT = gdb.parse_and_eval('INVALID_SEGMENT_WEIGHT'), gdb.parse_and_eval('INVALID_SEGMENT_WEIGHT')
        segment_weight = lambda it: str(it.dereference()) + \
            (' invalid' if it.dereference() == INVALID_SEGMENT_WEIGHT else ' max' if it.dereference() == MAX_SEGMENT_WEIGHT else '')


        result = ''
        print ('Graph has {} nodes and {} edges '.format(call(facade, 'GetNumberOfNodes'), call(facade, 'GetNumberOfEdges')))

        for node in range(call(facade, 'GetNumberOfNodes')):
            geometry_id = call(facade, 'GetGeometryIndex', node)
            direction = 'forward' if geometry_id['forward'] else 'reverse'
            geometry = get_by_geometry_id(geometry_id, 'Geometry')
            weights = get_by_geometry_id(geometry_id, 'Weights')

            ## add the edge-based node
            ref = 'n' + str(node)
            result += '<path id="' + ref + '" class="node" d="M' \
                                 + ' L'.join([t(lonlat(call(facade, 'GetCoordinateOfNode', x))) for x in it(geometry)]) \
                                 + '" />'
            result += '<text><textPath class="node ' + direction + '" xlink:href="#' + ref \
                                 + '" startOffset="60%">' + str(node) + '</textPath></text>\n'

            ## add segments with weights
            geometry_first = geometry['_M_impl']['_M_start']
            weights_first, weights_last = weights['_M_impl']['_M_start'], weights['_M_impl']['_M_finish']
            segment = 0
            while weights_first != weights_last:
                ref = 's' + str(node) + '.' + str(segment)
                result += '<path id="' + ref + '" class="segment" d="'  \
                                + 'M' + t(lonlat(call(facade, 'GetCoordinateOfNode', geometry_first.dereference()))) + ' ' \
                                + 'L' + t(lonlat(call(facade, 'GetCoordinateOfNode', (geometry_first+1).dereference()))) + '" />'\
                                + '<text class="segment weight ' + direction + '">'\
                                + '<textPath xlink:href="#' + ref + '" startOffset="50%">' \
                                + segment_weight(weights_first) + '</textPath></text>\n'
                weights_first += 1
                geometry_first += 1
                segment += 1

            ## add edge-based edges
            s0, s1 = geometry['_M_impl']['_M_start'].dereference(), (geometry['_M_impl']['_M_start'] + 1).dereference()
            for edge in range(call(facade, 'BeginEdges', node), call(facade, 'EndEdges', node)):
                target, edge_data = call(facade, 'GetTarget', edge), call(facade, 'GetEdgeData', edge)
                direction = 'both' if edge_data['forward'] and edge_data['backward'] else 'forward' if edge_data['forward'] else 'backward'
                target_geometry = get_by_geometry_id(call(facade, 'GetGeometryIndex', target), 'Geometry')
                t0, t1 = target_geometry['_M_impl']['_M_start'].dereference(), (target_geometry['_M_impl']['_M_start'] + 1).dereference()

                ## first control point
                s0x, s0y = lonlat(call(facade, 'GetCoordinateOfNode', s0))
                s1x, s1y = lonlat(call(facade, 'GetCoordinateOfNode', s1))
                d0x, d0y = s1x - s0x, s1y - s0y
                c0x, c0y = s0x + d0x /2 - d0y /4, s0y + d0y / 2 + d0x /4

                ## end point
                t0x, t0y = lonlat(call(facade, 'GetCoordinateOfNode', t0))
                t1x, t1y = lonlat(call(facade, 'GetCoordinateOfNode', t1))
                d1x, d1y = t1x - t0x, t1y - t0y
                e1x, e1y = t0x + d1x / 2, t0y + d1y / 2

                ## second control point
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
            width, height = 2100, 1600
            val = gdb.parse_and_eval(arg)
            svg = self.to_svg[str(val.type)](val, width, height)
            self.show_svg(svg, width, height)
        except KeyError as e:
            print ('no SVG printer for: ' + str(e))
        except gdb.error as e:
            print('error: ' % (e.args[0] if len(e.args)>0 else 'unspecified'))

SVGPrinter()
