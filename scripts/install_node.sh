# here we set up the node version on the fly. currently only node 4, but can be used for more values if need be
# This is done manually so that the build works the same on OS X
rm -rf ~/.nvm/ && git clone --depth 1 --branch v0.30.1 https://github.com/creationix/nvm.git ~/.nvm
source ~/.nvm/nvm.sh
nvm install $1
nvm use $1
node --version
npm --version
which node
