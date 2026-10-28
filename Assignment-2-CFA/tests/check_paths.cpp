// Compile the submitted implementation, including its original entry point.
#define main cfga_original_main
#include "../CFGA.cpp"
#undef main
#include <cstdlib>
#include <iostream>
#include <stdexcept>

class CheckedAnalysis : public CFGAnalysis
{
public:
    using CFGAnalysis::CFGAnalysis;

    void check(ICFG *icfg, unsigned expected)
    {
        analyze(icfg);
        const auto first = reachablePaths;
        require(callStack.empty(), "analysis left an unbalanced call stack");
        require(first.size() == expected, "unexpected number of paths");
        for (const auto &path : first)
        {
            require(!path.empty(), "empty path");
            require(sources.count(path.front()), "path does not start at main entry");
            require(sinks.count(path.back()), "path does not end at main exit");
            std::vector<unsigned> calls;
            for (size_t i = 1; i < path.size(); ++i)
            {
                const ICFGEdge *found = nullptr;
                for (const auto *edge : icfg->getICFGNode(path[i - 1])->getOutEdges())
                    if (edge->getDstNode()->getId() == path[i])
                    {
                        require(!found, "ambiguous graph edge");
                        found = edge;
                    }
                require(found, "path uses a nonexistent graph edge");
                if (const auto *call = SVFUtil::dyn_cast<CallCFGEdge>(found))
                    calls.push_back(call->getCallSite()->getId());
                else if (const auto *ret = SVFUtil::dyn_cast<RetCFGEdge>(found))
                {
                    require(!calls.empty(), "return without a call");
                    require(calls.back() == ret->getCallSite()->getId(),
                            "return belongs to a different call site");
                    calls.pop_back();
                }
            }
            require(calls.empty(), "path ends with outstanding calls");
        }
        analyze(icfg);
        require(reachablePaths == first && callStack.empty(),
                "repeated analysis changed results or leaked stack state");
        std::cout << "PASS: " << first.size()
                  << " paths; graph edges, call/return pairing, and repeatability verified\n";
    }

private:
    static void require(bool condition, const char *message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }
};

int main(int argc, char **argv)
{
    const char *expected = std::getenv("CFGA_EXPECTED_PATHS");
    if (!expected)
        return 2;
    auto modules = OptionBase::parseOptions(argc, argv, "CFGA regression", "<input-bitcode>");
    LLVMModuleSet::buildSVFModule(modules);
    SVFIRBuilder builder;
    auto *icfg = builder.build()->getICFG();
    int result = 0;
    try
    {
        CheckedAnalysis(icfg).check(icfg, std::stoul(expected));
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        result = 1;
    }
    LLVMModuleSet::releaseLLVMModuleSet();
    return result;
}
