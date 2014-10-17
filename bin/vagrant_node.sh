curl https://raw.githubusercontent.com/creationix/nvm/v0.17.2/install.sh | bash
source ~/.nvm/nvm.sh

nvm install 0.10
nvm install 0.11

nvm exec 0.10 npm install -g grunt-cli
nvm exec 0.11 npm install -g grunt-cli

nvm alias default 0.10

