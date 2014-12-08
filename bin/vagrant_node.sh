if ! [ -e ~/.nvm/nvm.sh ]; then
  curl https://raw.githubusercontent.com/creationix/nvm/v0.17.2/install.sh | bash
fi
source ~/.nvm/nvm.sh

nvm list 0.10 || nvm install 0.10
nvm exec 0.10 which grunt || nvm exec 0.10 npm install -g grunt-cli jasmine

nvm list 0.11 || nvm install 0.11
nvm exec 0.11 which grunt || nvm exec 0.11 npm install -g grunt-cli jasmine

nvm alias default 0.10
