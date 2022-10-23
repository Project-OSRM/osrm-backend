import querystring from 'querystring';
import { Format } from './Format';
import Ajv from 'ajv';

function makeAnnotationsSchema(allowedAnnotations: string[]): any {
    return {
        oneOf: [
            {
                type: 'array',
                items: {
                    enum: allowedAnnotations
                }
            },
            {
                type: 'boolean'
            }
        ],
        default: false
    };
}

const queryStringJsonSchemaGeneral = {
    type: 'object',
    properties: {
        // TODO: check numbers of elements is the same in bearings and radiuses
        bearings: {
            type: 'array',
            items: {
                type: 'array',
                // TODO: check [min;max]
                items: {
                    type: 'number'
                }
            }
        },
        radiuses: {
            type: 'array',
            items: {
                anyOf: [
                    {
                        type: 'number',
                        exclusiveMinimum: 0
                    },
                    {
                        type: 'string',
                        enum: ['unlimited']
                    }
                ]
            }
        },
        generate_hints: { type: 'boolean', default: true },
        hints: {
            type: 'array',
            items: {
                type: 'string'
            }
        },
        approaches: {
            type: 'array',
            items: {
                // TODO: default?
                enum: ['curb', 'unrestricted']
            }
        },
        exclude: {
            type: 'array',
            items: {
                type: 'string'
            }
        },
        snapping: {
            enum: ['any', 'default'],
            default: 'default'
        },
        skip_waypoints: { type: 'boolean', default: false },
    }
};

const queryStringJsonSchemaRoute = {
    type: 'object',
    properties: {
        ...queryStringJsonSchemaGeneral.properties,
        // TODO: strict mode: use allowUnionTypes to allow union
        alternatives: { type: ['boolean', 'integer'], default: false },
        steps: { type: 'boolean', default: false },
        annotations: makeAnnotationsSchema(['duration', 'nodes', 'distance', 'weight', 'datasources', 'speed']),
        geometries: {
            enum: ['polyline', 'polyline6', 'geojson'],
            default: 'polyline'
        },
        overview: {
            enum: ['simplified', 'full', 'false'],
            default: 'simplified'
        },
        continue_straight: {
            enum: ['default', 'true', 'false'],
            default: 'default'
        },
        waypoints: {
            type: 'array',
            items: {
                type: 'integer'
            }
        }
    }
};

const queryStringJsonSchemaNearest = {
    type: 'object',
    properties: {
        ...queryStringJsonSchemaGeneral.properties,
        number: { type: ['integer'], default: 1 }
    }
};

const queryStringJsonSchemaTable = {
    type: 'object',
    properties: {
        ...queryStringJsonSchemaGeneral.properties,
        sources: {
            anyOf: [
                {
                    type: 'array',
                    items: {
                        type: 'integer',
                        minimum: 0
                    }
                },
                {
                    type: 'string',
                    enum: ['all']
                }
            ]

        },
        destinations: {
            anyOf: [
                {
                    type: 'array',
                    items: {
                        type: 'integer',
                        minimum: 0
                    }
                },
                {
                    type: 'string',
                    enum: ['all']
                }
            ]

        },
        annotations: {
            type: 'array',
            items: {
                enum: ['duration', 'distance'],
                default: 'duration'
            }
        },
        fallback_speed: {
            type: 'number',
            exclusiveMinimum: 0
        },
        fallback_coordinate: {
            enum: ['input', 'snapped'],
            default: 'input'
        },
        scale_factor: {
            type: 'number',
            exclusiveMinimum: 0
        }
    }
};


const queryStringJsonSchemaMatch = {
    type: 'object',
    properties: {
        ...queryStringJsonSchemaGeneral.properties,

        steps: { type: 'boolean', default: false },
        geometries: {
            enum: ['polyline', 'polyline6', 'geojson'],
            default: 'polyline'
        },
        overview: {
            enum: ['simplified', 'full', 'false'],
            default: 'simplified'
        },
        timestamps: {
            type: 'array',
            items: {
                type: 'integer'
            }
        },

        gaps: {
            enum: ['split', 'ignore'],
            default: 'split'
        },
        tidy: { type: 'boolean', default: false },
        waypoints: {
            type: 'array',
            items: {
                type: 'integer'
            }
        },
        annotations: makeAnnotationsSchema(['duration', 'nodes', 'distance', 'weight', 'datasources', 'speed'])
    }
};

const queryStringJsonSchemaTrip = {
    type: 'object',
    properties: {
        ...queryStringJsonSchemaGeneral.properties,

        roundtrip: { type: 'boolean', default: true },
        source: {
            enum: ['any', 'first'],
            default: 'any'
        },
        destination: {
            enum: ['any', 'last'],
            default: 'any'
        },

        annotations: makeAnnotationsSchema(['duration', 'nodes', 'distance', 'weight', 'datasources', 'speed']),
        steps: { type: 'boolean', default: false },
        geometries: {
            enum: ['polyline', 'polyline6', 'geojson'],
            default: 'polyline'
        },
        overview: {
            enum: ['simplified', 'full', 'false'],
            default: 'simplified'
        }
    }
};

const paramsJsonSchema = {
    type: 'object',
    properties: {
        coordinatesAndFormat: { 
            type: 'string'
        }
    }
};

export const routeSchema = {
    querystring: queryStringJsonSchemaRoute,
    params: paramsJsonSchema
};


export const nearestSchema = {
    querystring: queryStringJsonSchemaNearest,
    params: paramsJsonSchema
};

export const tableSchema = {
    querystring: queryStringJsonSchemaTable,
    params: paramsJsonSchema
};

export const matchSchema = {
    querystring: queryStringJsonSchemaMatch,
    params: paramsJsonSchema
};
export const tripSchema = {
    querystring: queryStringJsonSchemaTrip,
    params: paramsJsonSchema
};

const paramsJsonSchemaTile = {
    type: 'object',
    properties: {
        z: { type: 'integer', minimum: 12, maximum: 22 },
        x: { type: 'integer', minimum: 0 },
        y: { type: 'integer', minimum: 0 },
    },
};

export const tileSchema = {
    params: paramsJsonSchemaTile,
};

function parseArray(listString: string | string[], separator: string): string[] {
    // `querystring` parses `foo=1&foo=2` as `{ foo: ['1', '2'] }`
    if (Array.isArray(listString)) {
        return listString;
    }
    return listString.split(separator);
}

export function parseQueryString(queryString: string): any {
    const parsed: any = querystring.parse(queryString, '&', '=', {
        // 0 means "infinity"
        maxKeys: 0
    });
    for (const key of ['timestamps', 'radiuses', 'approaches', 'waypoints', 'hints']) {
        if (key in parsed) {
            parsed[key] = parseArray(parsed[key], ';');
        }
    }
    for (const key of ['sources', 'destinations']) {
        if (key in parsed && parsed[key] !== 'all') {
            parsed[key] = parseArray(parsed[key], ';');
        }
    }
    if ('exclude' in parsed) {
        parsed['exclude'] = parseArray(parsed['exclude'], ',');
    }
    if ('bearings' in parsed) {
        parsed['bearings'] = parseArray(parsed['bearings'], ';').map(bearingWithRange => parseArray(bearingWithRange, ',').filter(bearing => bearing !== ''));
    }
    if ('annotations' in parsed) {
        if (!['true', 'false'].includes(parsed['annotations'])) {
            parsed['annotations'] = parseArray(parsed['annotations'], ',');
        }
    }

    return parsed;
}

const coordinatesSchema = new Ajv({ allErrors: true, coerceTypes: true }).compile({
    type: 'array',
    items: {
        type: 'array',
        items: {
            type: 'number',
            // TODO: ranges
            minimum: -180,
            maximum: 180
        },
        minItems: 2,
        maxItems: 2
    },
    minItems: 1
});



export function parseCoordinatesAndFormat(coordinatesAndFormat: string): { coordinates: [number, number][], format: Format } {
    let format: Format = Format.Json;
    
    // try to handle case when we have format(i.e. `.flatbuffers` or `.json`) at the end
    const lastDotIndex = coordinatesAndFormat.lastIndexOf('.');
    if (lastDotIndex > 0) {
        const formatString = coordinatesAndFormat.slice(lastDotIndex);
        if (formatString == '.flatbuffers' || formatString == '.json') {
            coordinatesAndFormat = coordinatesAndFormat.slice(0, lastDotIndex);
            format = formatString == '.flatbuffers' ? Format.Flatbuffers : Format.Json;
        }
    }

    const coordinates: any = coordinatesAndFormat.split(';').map(c => c.split(','));
    if (!coordinatesSchema(coordinates)) {
        throw {message: 'Invalid coordinates', code: 'InvalidQuery'};
    }
    return { coordinates: coordinates as [number, number][], format };
}
