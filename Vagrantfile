# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure("2") do |config|
  config.vm.provision "shell", inline: <<-SHELL
    export DEBIAN_FRONTEND=noninteractive
    apt-get update -q
    apt-get install -q -y \
      -o Dpkg::Options::="--force-confdef" \
      -o Dpkg::Options::="--force-confold" \
      ansible git

    ansible-playbook /vagrant/bootstrap/playbook.yml
  SHELL

  config.vm.define :bionic, autostart: false do |bionic|
    bionic.vm.box = "ubuntu/bionic64"
  end

  config.vm.define :cosmic, autostart: false do |cosmic|
    cosmic.vm.box = "ubuntu/cosmic64"
  end
end
