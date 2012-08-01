/**
 *  @file
 *  @author     2012 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *  @copyright  Simplified BSD
 *
 *  @cond
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 *  @endcond
 */

#include "umundo/protoc-rpc/ServiceGeneratorJava.h"

#include <set>
#include <iostream>

#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/compiler/java/java_generator.h>
#include <google/protobuf/compiler/java/java_file.h>
#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/descriptor.pb.h>

namespace umundo {

using namespace google::protobuf::compiler::java;

bool ServiceGeneratorJava::Generate(const FileDescriptor* file,
                                    const string& parameter,
                                    GeneratorContext* generator_context,
                                    string* error) const {
	vector<pair<string, string> > options;
	ParseGeneratorParameter(parameter, &options);

	FileGenerator file_generator(file);
	if (!file_generator.Validate(error)) {
		return false;
	}

	string basename = StripProto(file->name());
	string package_dir = JavaPackageToDir(file_generator.java_package());

	if (file->service_count() == 0)
		return true;

	for (int i = 0; i < file->service_count(); i++) {
		const ServiceDescriptor* svcDesc = file->service(i);

		std::set<string> inTypes;
		std::set<string> outTypes;
		std::set<string>::iterator inTypeIter;
		std::set<string>::iterator outTypeIter;

		if (svcDesc->method_count() > 0) {
			for (int i = 0; i < svcDesc->method_count(); i++) {
				const MethodDescriptor* methodDesc = svcDesc->method(i);
				inTypes.insert(ClassName(methodDesc->input_type()));
				outTypes.insert(ClassName(methodDesc->output_type()));
			}
		}

		// Generate Stub
		{
			scoped_ptr<io::ZeroCopyOutputStream> output(generator_context->Open(package_dir + svcDesc->name() + "Stub.java"));
			io::Printer printer(output.get(), '$');

			// header prolog and imports
			printer.Print(
			    "package $package$;\n\n"
			    "// Generated by the umundo protocol buffer compiler. DO NOT EDIT!\n"
			    "// source: $filename$\n\n"
			    "import org.umundo.rpc.ServiceDescription;\n"
			    "import org.umundo.rpc.ServiceStub;\n\n",
			    "package", file_generator.java_package(),
			    "filename", file->name());

			// class declaration
			printer.Print(
			    "public class $servicename$Stub extends ServiceStub {\n",
			    "servicename", svcDesc->name());

			// constructor
			printer.Print(
			    "\tpublic $servicename$Stub(ServiceDescription svcDesc) {\n"
			    "\t\tsuper(svcDesc);\n",
			    "servicename", svcDesc->name());

			// register types
			for (outTypeIter = outTypes.begin(); outTypeIter != outTypes.end(); outTypeIter++) {
				printer.Print(
				    "\t\t_rpcSub.registerType($outType$.class);\n",
				    "outType", (*outTypeIter)
				);
			}

			// end constructor
			printer.Print("\t}\n\n");

			// stub method dispatchers
			for (int i = 0; i < svcDesc->method_count(); i++) {
				const MethodDescriptor* methodDesc = svcDesc->method(i);
				printer.Print(
				    "\tpublic $outType$ $methodName$($inType$ in) throws InterruptedException {\n"
				    "\t\treturn ($outType$) callStubMethod",
				    "methodName", methodDesc->name(),
				    "inType", ClassName(methodDesc->input_type()),
				    "outType", ClassName(methodDesc->output_type())
				);

				printer.Print(
				    "(\"$methodName$\", in, \"$inType$\", \"$outType$\");\n"
				    "\t}\n\n",
				    "methodName", methodDesc->name(),
				    "inType", methodDesc->input_type()->name(),
				    "outType", methodDesc->output_type()->name()
				);

			}

			// end class declaration
			printer.Print("}\n");
		}

		// Generate Service base class
		{
			scoped_ptr<io::ZeroCopyOutputStream> output(generator_context->Open(package_dir + svcDesc->name() + ".java"));
			io::Printer printer(output.get(), '$');

			// header prolog and imports
			printer.Print(
			    "package $package$;\n\n"
			    "// Generated by the umundo protocol buffer compiler. DO NOT EDIT!\n"
			    "// source: $filename$\n\n"
			    "import com.google.protobuf.MessageLite;\n"
			    "import org.umundo.rpc.Service;\n\n",
			    "package", file_generator.java_package(),
			    "filename", file->name());

			// class declaration
			printer.Print(
			    "public abstract class $servicename$ extends Service {\n",
			    "servicename", svcDesc->name());

			// constructor
			printer.Print(
			    "\tprotected $servicename$() {\n"
			    "\t\t_serviceName = \"$servicename$\";\n",
			    "servicename", svcDesc->name());

			// register types
			for (inTypeIter = inTypes.begin(); inTypeIter != inTypes.end(); inTypeIter++) {
				printer.Print(
				    "\t\t_rpcSub.registerType($inType$.class);\n",
				    "inType", (*inTypeIter)
				);
			}

			// end constructor
			printer.Print("\t}\n\n");

			// abstract method declarations
			for (int i = 0; i < svcDesc->method_count(); i++) {
				const MethodDescriptor* methodDesc = svcDesc->method(i);
				printer.Print(
				    "\tpublic abstract $outType$ $methodName$($inType$ req);\n",
				    "methodName", methodDesc->name(),
				    "outType", ClassName(methodDesc->output_type()),
				    "inType", ClassName(methodDesc->input_type())
				);
			}
			printer.Print("\n");

			// metod dispatcher
			printer.Print(
			    "\t@SuppressWarnings(\"unused\")\n"
			    "\t@Override\n"
			    "\tpublic Object callMethod(String methodName, MessageLite in, String inType, String outType) throws InterruptedException {\n"
			    "\t\tif (false) {\n"
			);

			for (int i = 0; i < svcDesc->method_count(); i++) {
				const MethodDescriptor* methodDesc = svcDesc->method(i);
				printer.Print(
				    "\t\t} else if (methodName.compareTo(\"$methodName$\") == 0) {\n"
				    "\t\t\treturn $methodName$(($inType$)in);\n",
				    "methodName", methodDesc->name(),
				    "inType", ClassName(methodDesc->input_type())
				);
			}
			// method dispatcher end
			printer.Print(
			    "\t\t}\n"
			    "\t\treturn null;\n"
			    "\t}\n");

			// end class declaration
			printer.Print("}\n");

		}
	}

	return true;

}

} // namespace end

int main(int argc, char* argv[]) {
	umundo::ServiceGeneratorJava generator;
	return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
