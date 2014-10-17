def run_bin bin, options
  Dir.chdir TEST_FOLDER do
    opt = options.dup
    
    if opt.include? '{base}'
      raise "*** {base} is missing" unless prepared_file
      opt.gsub! "{base}", "#{prepared_file}" 
    end

    if opt.include? '{profile}'
      opt.gsub! "{profile}", "#{PROFILES_PATH}/#{@profile}.lua" 
    end

    cmd = "#{QQ}#{BIN_PATH}/#{bin}#{EXE}#{QQ} #{opt} 2>error.log"
    @stdout = `#{cmd}`
    @stderr = File.read 'error.log'
    @exit_code = $?.exitstatus
  end
end