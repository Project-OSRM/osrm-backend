module.exports = {
    default: '--strict --tags ~@stress --tags ~@todo --tags ~@mld --require features/support --require features/step_definitions',
    ch: '--strict --tags ~@stress --tags ~@todo --tags ~@mld -f progress --require features/support --require features/step_definitions',
    todo: '--strict --tags @todo --require features/support --require features/step_definitions',
    all: '--strict --require features/support --require features/step_definitions',
    mld: '--strict --tags ~@stress --tags ~@todo --tags ~@ch --require features/support --require features/step_definitions -f progress',
    verify_routed_js: '--strict --tags ~@skip_on_routed_js --tags ~@stress --tags ~@todo --tags ~@mld-only -f progress --require features/support --require features/step_definitions',
    mld_routed_js: '--strict --tags ~@skip_on_routed_js --tags ~@stress --tags ~@todo --tags ~@ch --require features/support --require features/step_definitions -f progress',
};
