module.exports = function () {
    this.updateFingerprintExtract = (str) => {
        this.fingerprintExtract = this.hashString([this.fingerprintExtract, str].join('-'));
    };

    this.updateFingerprintContract = (str) => {
        this.fingerprintContract = this.hashString([this.fingerprintContract, str].join('-'));
    };

    this.setProfile = (profile, cb) => {
        this.profileHash = this.DEPENDENCY_HASHES[this.getProfilePath(profile)];
        this.updateFingerprintExtract(this.profileHash);
        cb();
    };

    this.setExtractArgs = (args, callback) => {
        this.extractArgs = args;
        this.updateFingerprintExtract(args);
        callback();
    };

    this.setContractArgs = (args, callback) => {
        this.contractArgs = args;
        this.updateFingerprintContract(args);
        callback();
    };
};
