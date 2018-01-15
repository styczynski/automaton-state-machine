Vagrant.configure("2") do |config|
  config.vm.define :debian_jessie do |config|
    config.vm.box = "debian_jessie"

    config.vm.network "private_network", ip: "192.168.50.50"

    config.vm.provider "virtualbox" do |vb|
        vb.customize ["modifyvm", :id, "--natdnshostresolver1", "on"]
    end

    config.vm.synced_folder ".", "/finite-automaton", type: "nfs", nfs_udp: false
  end
end
