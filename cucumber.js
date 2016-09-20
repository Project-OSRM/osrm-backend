module.exports = {
    default: '--require features --tags ~@stress --tags ~@todo',
    verify: '--require features --tags ~@todo --tags ~@bug --tags ~@stress -f progress',
    jenkins: '--require features --tags ~@todo --tags ~@bug --tags ~@stress --tags ~@options -f progress',
    bugs: '--require features --tags @bug',
    todo: '--require features --tags @todo',
    all: '--require features'
}



