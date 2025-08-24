// Default profile
export default {
  strict: true,
  tags: 'not @stress and not @todo and not @mld',
  import: ['features/support', 'features/step_definitions'],
};

// Additional profiles  
export const ch = {
  strict: true,
  tags: 'not @stress and not @todo and not @mld',
  format: ['progress'],
  import: ['features/support', 'features/step_definitions'],
};

export const todo = {
  strict: true,
  tags: '@todo',
  import: ['features/support', 'features/step_definitions'],
};

export const all = {
  strict: true,
  import: ['features/support', 'features/step_definitions'],
};

export const mld = {
  strict: true,
  tags: 'not @stress and not @todo and not @ch',
  format: ['progress'],
  import: ['features/support', 'features/step_definitions'],
};
