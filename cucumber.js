module.exports = {
  default: {
    strict: true,
    tags: '~@stress and ~@todo and ~@mld',
    require: ['features/support', 'features/step_definitions'],
  },
  ch: {
    strict: true,
    tags: '~@stress and ~@todo and ~@mld',
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
    tags: '~@stress and ~@todo and ~@ch',
    format: ['progress'],
    require: ['features/support', 'features/step_definitions'],
  },
};
