// See: https://github.com/cucumber/cucumber-js/blob/main/docs/profiles.md

export default function() {
  const penv = process.env;

  function int(s, def) {
    return (s && parseInt(s)) || def;
  }

  function str(s, def) {
    return s || def;
  }

  const commonWorldParameters = {
    httpTimeout:  int(penv.CUCUMBER_HTTP_TIMEOUT,  2000), // must be less than default timeout
    testPath:     str(penv.CUCUMBER_TEST_PATH,     'test'),
    profilesPath: str(penv.CUCUMBER_PROFILES_PATH, 'profiles'),
    logsPath:     str(penv.CUCUMBER_LOGS_PATH,     'test/logs'),
    cachePath:    str(penv.CUCUMBER_CACHE_PATH,    'test/cache'),
    buildPath:    str(penv.OSRM_BUILD_DIR,         'build'),
    loadMethod:   str(penv.OSRM_LOAD_METHOD,       'datastore'),
    algorithm:    str(penv.OSRM_ALGORITHM,         'ch'),
    port:         int(penv.OSRM_PORT,              5000),
    ip:           str(penv.OSRM_IP,                '127.0.0.1'),
  }

  const baseConfig = {
    strict: true,
    import: [
      'features/support/',
      'features/step_definitions/',
      'features/lib/'
    ],
    worldParameters : commonWorldParameters,
    tags: 'not @stress and not @todo',
  }

  function wp(worldParameters) {
    return { worldParameters: worldParameters };
  }

  const htmlReportFilename = `test/logs/cucumber-${process.env.algorithm}-${process.env.loadmethod}.report.html`

  return {
    // Default profile
    default: {
      ... baseConfig,
    },

    // base configs
    home: {
      ... baseConfig,
      format: [
        'progress-bar',
        ['html', htmlReportFilename]
      ],
    },
    github: {
      ... baseConfig,
      format: [
        './features/lib/github_summary_formatter.js',
        ['html', htmlReportFilename]
      ],
      publish: true
    },

    // patches to base configs
    stress: { tags: '@stress'},
    todo:   { tags: '@todo'},
    all:    { tags: '' },

    // algorithms
    ch:  wp({algorithm: 'ch'}),
    mld: wp({algorithm: 'mld'}),

    // data load methods
    datastore: wp({loadMethod: 'datastore'}),
    directly:  wp({loadMethod: 'directly'}),
    mmap:      wp({loadMethod: 'mmap'}),
  }
};
