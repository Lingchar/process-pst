require File.join(File.dirname(__FILE__), 'spec_helper')

require 'assert2/xpath'

describe "process-pst" do
  include Test::Unit::Assertions # for assert2/xpath

  after do
    rm_rf(build_path("out"))
  end

  def loadfile
    build_path("out/edrm-loadfile.xml")
  end

  it "should fail if passed invalid arguments" do
    process_pst("nosuch.pst", "out").should == false
  end

  it "should fail if the output directory exists" do
    mkdir_p(build_path("out"))
    process_pst("pstsdk/test/sample1.pst", "out").should == false
  end

  context "sample1.pst" do
    before do
      @result = process_pst("pstsdk/test/sample1.pst", "out")
      _assert_xml(File.read(loadfile))
    end

    it "should succeed if passed valid arguments" do
      @result.should == true
    end

    it "should create a directory and edrm loadfile" do
      File.exist?(loadfile).should == true
    end

    it "should generate a valid EDRM loadfile" do
      xpath("/Root[@DataInterchangeType='Update']/Batch") do
        xpath("./Documents") { true }
        xpath("./Relationships") { true }
      end
    end

    it "should output metadata for each message and attachment in the PST" do
      xpath("//Document[@DocID='d0000001'][@DocType='Message']/Tags") do
        xpath("./Tag[@TagName='#Subject']" +
              "[@TagValue='Here is a sample message']" +
              "[@TagDataType='Text']") { true }
      end
      xpath("//Document[@DocID='d0000002'][@DocType='File']/Tags") do
        xpath("./Tag[@TagName='#FileName']" +
              "[@TagValue='leah_thumper.jpg']" +
              "[@TagDataType='Text']") { true }
      end      
    end
  end

  context "four_nesting_levels.pst" do
    before do
      process_pst("test_data/four_nesting_levels.pst", "out")
      _assert_xml(File.read(loadfile))       
    end

    it "should recurse through nested submessages" do
      subjects = ["Outermost message", "Middle message", "Innermost message"]
      subjects.each do |s|
        xpath("//Tag[@TagName='#Subject'][@TagValue='#{s}']") { true }
      end

      filenames = ["hello.txt"]
      filenames.each do |s|
        xpath("//Tag[@TagName='#FileName'][@TagValue='#{s}']") { true }
      end
    end

    it "should output the actual attached files" do
      xpath("//Document/Files/File[@FileType='Native']") do
        xpath("./ExternalFile" +
              "[@FileName='d0000004.txt']" +
              "[@FileSize='15']" +
              "[@Hash='78016cea74c298162366b9f86bfc3b16']") { true }
      end
      # TODO: Actual file on disk
    end
  end
end
