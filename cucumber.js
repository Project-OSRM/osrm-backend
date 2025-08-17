module.exports = {
  default: {
    strict: true,
    tags: 'not @stress and not @todo and not @mld',
    require: ['features/support', 'features/step_definitions'],
  },
  ch: {
    strict: true,
    tags: 'not @stress and not @todo and not @mld',
    format: ['progress'],
    require: ['features/support', 'features/step_definitions'],
  },
  todo: {
    strict: true,
    tags: '@todo',
    require: ['features/support', 'features/step_definitions'],
  },
  all: {
    strict: true,
    require: ['features/support', 'features/step_definitions'],
  },
  mld: {
    strict: true,
    format: ['progress'],
    require: ['features/support', 'features/step_definitions'],
  },
};
