# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  # All Vagrant configuration is done here. The most common configuration
  # options are documented and commented below. For a complete reference,
  # please see the online documentation at vagrantup.com.
  config.vm.box = "chef/centos-6.5"
  config.vm.hostname = "node-gemfire-dev"
  config.vm.define "node-gemfire-dev"

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  # config.vm.network "forwarded_port", guest: 80, host: 8080

  config.vm.synced_folder ".", "/project"

  # View the documentation for the provider you're using for more
  # information on available options.
  config.vm.provider "virtualbox" do |vb|
    #vb.gui = true
    vb.name = "node-gemfire-dev"
    vb.customize ["modifyvm", :id, "--memory", "2048"]
  end

  config.vm.provision "shell", path: "bin/vagrant_setup_centos6.5.sh"
  config.vm.provision "shell", privileged: false, path: "bin/vagrant_build_project.sh"
end
