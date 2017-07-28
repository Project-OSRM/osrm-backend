module.exports = {
    default: '--strict --tags ~@stress --tags ~@mld --tags ~@todo --require features/support --require features/step_definitions',
    verify: '--strict --tags ~@stress --tags ~@mld --tags ~@todo -f progress --require features/support --require features/step_definitions',
    todo: '--strict --tags @todo --require features/support --require features/step_definitions',
    all: '--strict --require features/support --require features/step_definitions',
    mld: '--strict --tags ~@stress --tags ~@todo --tags ~@alternative --require features/support --require features/step_definitions -f progress'
};
