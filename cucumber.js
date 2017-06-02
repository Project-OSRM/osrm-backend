module.exports = {
    default: '--strict --tags ~@stress --tags ~@todo --tags ~@conditionals --require features/support --require features/step_definitions',
    verify: '--strict --tags ~@stress --tags ~@todo -f progress --require features/support --require features/step_definitions',
    todo: '--strict --tags @todo --require features/support --require features/step_definitions',
    all: '--strict --require features/support --require features/step_definitions',
    mld: '--strict --tags ~@stress --tags ~@todo --tags ~@alternative --tags ~@matrix --tags ~@trip --tags --require features/support --require features/step_definitions -f progress',
    conditionals: '--strict --tags @conditionals --require features/support --require features/step_definitions',
    mld_conditionals: '--strict --tags @conditionals --require features/support --require features/step_definitions -f progress'
}
