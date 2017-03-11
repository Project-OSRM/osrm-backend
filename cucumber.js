module.exports = {
    default: '--strict --tags ~@stress --tags ~@todo --require features/support --require features/step_definitions',
    verify: '--strict --tags ~@stress --tags ~@todo -f progress --require features/support --require features/step_definitions',
    todo: '--strict --tags @todo --require features/support --require features/step_definitions',
    all: '--strict --require features/support --require features/step_definitions',
    mld: '--strict --tags ~@stress --tags ~@todo --tags ~@match --tags ~@alternative --tags ~@traffic --tags ~@weight --tags ~@matrix --tags ~@trip --tags ~@via --require features/support --require features/step_definitions -f progress'
}
