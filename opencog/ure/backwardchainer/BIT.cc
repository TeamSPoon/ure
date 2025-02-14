/*
 * BIT.cc
 *
 * Copyright (C) 2016-2017 OpenCog Foundation
 * Author: Nil Geisweiller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <boost/range/algorithm/binary_search.hpp>
#include <boost/range/algorithm/lower_bound.hpp>
#include <boost/range/algorithm/reverse.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/join.hpp>

#include <opencog/util/random.h>
#include <opencog/util/algorithm.h>
#include <opencog/atoms/core/FindUtils.h>
#include <opencog/atoms/core/TypeUtils.h>
#include <opencog/atoms/grounded/LibraryManager.h>
#include <opencog/atoms/pattern/PatternUtils.h>

#include "BIT.h"
#include "../URELogger.h"

namespace opencog {

/////////////
// BITNode //
/////////////

BITNode::BITNode(const Handle& bd, const BITNodeFitness& fi)
	: body(bd), fitness(fi), exhausted(false) {
	complexity = -std::log(operator()());
}

double BITNode::operator()() const
{
	// The probably estimate is anti-proportional to the fitness. The
	// assumption used here is that, if the fitness is already high,
	// expanding the BIT-Node is less likely to increase it. This
	// assumption is perfectly right when the fitness equals its upper
	// bound, but isn't generally right otherwise.
	return (exhausted ? 0.0 : 1.0) *
		(fitness.upper - fitness(*this)) / (fitness.upper - fitness.lower);
}

std::string	BITNode::to_string(const std::string& indent) const
{
	std::stringstream ss;
	ss << indent << "body:" << std::endl
	   << oc_to_string(body, indent + OC_TO_STRING_INDENT) << std::endl
	   << indent << "exhausted: " << exhausted << std::endl
	   << indent << "rules:" << std::endl
	   << (indent + oc_to_string_indent) << "size = " << rules.size();
	std::string rule_indent = indent + oc_to_string_indent + oc_to_string_indent;
	size_t i = 0;
	for (const auto& rule : rules)
		ss << std::endl << (indent + oc_to_string_indent)
		   << "rule[" << i++ << "]:" << std::endl
		   << rule.first.to_short_string(rule_indent);
	return ss.str();
}

////////////
// AndBIT //
////////////

AndBIT::AndBIT() : complexity(0), exhausted(false), queried_as(nullptr) {}

AndBIT::AndBIT(AtomSpace& bit_as, const Handle& target, Handle vardecl,
               const BITNodeFitness& fitness, const AtomSpace* qas)
	: exhausted(false), queried_as(qas)
{
	// in case it is undefined
	if (nullptr == vardecl)
	{
		HandleSet vars = get_free_variables(target);
		vardecl = HandleCast(createVariableSet(HandleSeq(vars.begin(), vars.end())));
	}

	// Create initial FCS
	Handle body =
		Unify::remove_constant_clauses(vardecl, target, queried_as);
	HandleSeq bl{body, target};
	vardecl = filter_vardecl(vardecl, body); // remove useless vardecl
	if (vardecl)
		bl.insert(bl.begin(), vardecl);
	fcs = bit_as.add_link(BIND_LINK, std::move(bl));

	// Insert the initial BITNode and initialize the AndBIT complexity
	auto it = insert_bitnode(target, fitness);
	complexity = it->second.complexity;
}

AndBIT::AndBIT(const Handle& f, double cpx, const AtomSpace* qas)
	: fcs(f), complexity(cpx), exhausted(false), queried_as(qas)
{
	set_leaf2bitnode();         // TODO: might differ till needed to optimize
}

AndBIT::~AndBIT() {}

AndBIT AndBIT::expand(const Handle& leaf,
                      const RuleTypedSubstitutionPair& rule,
                      double prob) const
{
	Handle new_fcs = expand_fcs(leaf, rule);
	double new_cpx = expand_complexity(leaf, prob);

	// Only consider expansions that actually expands
	if (content_eq(fcs, new_fcs)) {
		ure_logger().warn() << "The new FCS is equal to the old one. "
		                    << "There is probably a bug. This expansion has "
		                    << "been cancelled.";
		return AndBIT();
	}

	// Discard expansion with cycle
	if (has_cycle(BindLinkCast(new_fcs)->get_implicand()[0])) {
		ure_logger().debug() << "The new FCS has some cycle (some conclusion "
		                     << "has itself has premise, directly or "
		                     << "indirectly). This expansion has been cancelled.";
		return AndBIT();
	}

	return AndBIT(new_fcs, new_cpx, queried_as);
}

BITNode* AndBIT::select_leaf()
{
	// Generate the distribution over target leaves according to the
	// BIT-node fitnesses. The higher the fitness the lower the chance
	// of being selected as it is already fit.
	std::vector<double> weights;
	bool all_weights_null = true;
	for (const auto& lb : leaf2bitnode) {
		double p = lb.second();
		weights.push_back(p);
		if (p > 0) all_weights_null = false;
	}

	// Check that the distribution is well defined
	if (all_weights_null)
		return nullptr;

	// If well defined then sample according to it
	LeafDistribution dist(weights.begin(), weights.end());
	return &rand_element(leaf2bitnode, dist).second;
}

void AndBIT::reset_exhausted()
{
	for (auto& el : leaf2bitnode)
		el.second.exhausted = false;
	exhausted = false;
}

bool AndBIT::has_cycle() const
{
	return has_cycle(BindLinkCast(fcs)->get_implicand()[0]);
}

bool AndBIT::has_cycle(const Handle& h, HandleSet ancestors) const
{
	if (h->get_type() == EXECUTION_OUTPUT_LINK) {
		Handle arg = h->getOutgoingAtom(1);
		if (arg->get_type() == LIST_LINK) {
			Handle conclusion = arg->getOutgoingAtom(0);
			if (contains(ancestors, conclusion))
				return true;

			ancestors.insert(conclusion);
			Arity arity = arg->get_arity();
			if (1 < arity) {
				bool unordered_premises =
					arg->getOutgoingAtom(1)->get_type() == SET_LINK;
				if (unordered_premises) {
					OC_ASSERT(arity == 2,
					          "Mixture of ordered and unordered"
					          " premises not implemented!");
					arg = arg->getOutgoingAtom(1);
					for (const Handle& ph : arg->getOutgoingSet())
						if (has_cycle(ph, ancestors))
							return true;
					return false;
				} else {
					for (Arity i = 1; i < arity; ++i) {
						Handle ph = arg->getOutgoingAtom(i);
						if (has_cycle(ph, ancestors))
							return true;
						return false;
					}
				}
			}
			return false;
		} else {
			return contains(ancestors, arg);
		}
	} else {
		return contains(ancestors, h);
	}
}

bool AndBIT::operator==(const AndBIT& andbit) const
{
	return fcs == andbit.fcs;
}

bool AndBIT::operator<(const AndBIT& andbit) const
{
	// Sort by complexity to so that simpler and-BITs come first. Then
	// by content. Makes it easier to prune by complexity. It should
	// also make sampling a bit faster. And finally the user probabably
	// want that.
	return (complexity < andbit.complexity)
		or (complexity == andbit.complexity
		    and content_based_handle_less()(fcs, andbit.fcs));
}

std::string AndBIT::to_string(const std::string& indent) const
{
	return oc_to_string(fcs, indent);
}

std::string AndBIT::fcs_to_ascii_art(const Handle& nfcs) const
{
	return fcs_rewrite_to_ascii_art(BindLinkCast(nfcs)->get_implicand()[0]);
}

std::string AndBIT::fcs_rewrite_to_ascii_art(const Handle& h) const
{
	if (h->get_type() == EXECUTION_OUTPUT_LINK) {
		Handle gsn = h->getOutgoingAtom(0);
		Handle arg = h->getOutgoingAtom(1);
		if (arg->get_type() == LIST_LINK) {
			// Render the conclusion
			Handle conclusion = arg->getOutgoingAtom(0);
			std::string conclusion_aa = fcs_rewrite_to_ascii_art(conclusion);

			// Render the premises
			Arity arity = arg->get_arity();
			if (1 < arity) {
				std::vector<std::string> premises_aas;
				bool unordered_premises =
					arg->getOutgoingAtom(1)->get_type() == SET_LINK;
				if (unordered_premises) {
					OC_ASSERT(arity == 2,
					          "Mixture of ordered and unordered"
					          " premises not implemented!");
					arg = arg->getOutgoingAtom(1);
					for (const Handle& ph : arg->getOutgoingSet())
						premises_aas.push_back(fcs_rewrite_to_ascii_art(ph));
				} else {
					for (Arity i = 1; i < arity; ++i) {
						Handle ph = arg->getOutgoingAtom(i);
						premises_aas.push_back(fcs_rewrite_to_ascii_art(ph));
					}
				}
				// Merge horizontally the ascii arts of all premises
				std::string premises_merged_aa = ascii_art_hmerge(premises_aas);

				// Put a line over the head of the conclusion, with
				// the premises over that line.
				std::string ul(line_separator(premises_merged_aa, conclusion_aa,
				                              gsn, unordered_premises));
				unsigned ulls = leading_spaces(ul);
				unsigned ullls = ul.size() + ulls;
				unsigned conclusion_offset = ullls < conclusion_aa.size() ? 0 :
					(ullls - conclusion_aa.size()) / 2;
				std::string conclusion_indent(conclusion_offset, ' ');
				return premises_merged_aa + "\n" + ul + "\n"
					+ conclusion_indent + conclusion_aa;
			} else {
				// No premises, just put a line over the head of the
				// conclusion
				std::string line_str(line_separator("", conclusion_aa, gsn));
				return line_str + "\n" + conclusion_aa;
			}
		} else {
			// No premise, put a line over the head of the conclusion
			std::string conclusion_aa = fcs_rewrite_to_ascii_art(arg);
			std::string line_str(line_separator("", conclusion_aa, gsn));
			return line_str + "\n" + conclusion_aa;
		}
	} else return h->id_to_string();
}

double AndBIT::expand_complexity(const Handle& leaf, double prob) const
{
	// Calculate the complexity of the expanded and-BIT. Sum up the
	// complexity of the parent and-BIT with the complexity of the
	// expanded BIT-node and the complexity of the rule (1 - log(prob))
	return complexity
		+ leaf2bitnode.find(leaf)->second.complexity
		+ 1 - log(prob);
}

Handle AndBIT::expand_fcs(const Handle& leaf,
                          const RuleTypedSubstitutionPair& rule) const
{
	// Unify the rule conclusion with the leaf, and substitute any
	// variables in it by the associated term.
	Handle nfcs = substitute_unified_variables(leaf, rule.second);

	BindLinkPtr nfcs_bl(BindLinkCast(nfcs));
	Handle nfcs_vardecl = nfcs_bl->get_vardecl();
	Handle nfcs_pattern = nfcs_bl->get_body();
	Handle nfcs_rewrite = nfcs_bl->get_implicand()[0]; // assume that there is only one
	Handle rule_vardecl = rule.first.get_vardecl();

	// Generate new pattern term
	Handle npattern = expand_fcs_pattern(nfcs_pattern, rule.first);

	// Generate new rewrite term
	Handle nrewrite = expand_fcs_rewrite(nfcs_rewrite, rule.first);

	// Generate new vardecl
	// TODO: is this merging necessary?
	Handle merged_vardecl = merge_vardecl(nfcs_vardecl, rule_vardecl);
	Handle nvardecl = filter_vardecl(merged_vardecl, {npattern, nrewrite});

	// Remove constant clauses from npattern
	npattern = Unify::remove_constant_clauses(nvardecl, npattern, queried_as);

	// Generate new atomese forward chaining s trategy
	HandleSeq noutgoings({npattern, nrewrite});
	if (nvardecl)
		noutgoings.insert(noutgoings.begin(), nvardecl);
	nfcs = fcs->getAtomSpace()->add_link(BIND_LINK, std::move(noutgoings));

	// Log expansion
	LAZY_URE_LOG_DEBUG << "Expanded forward chainer strategy:" << std::endl
	                   << nfcs->to_string();
	LAZY_URE_LOG_DEBUG << "With inference tree:" << std::endl << std::endl
	                   << fcs_to_ascii_art(nfcs) << std::endl;

	return nfcs;
}

void AndBIT::set_leaf2bitnode()
{
	// For each leaf of fcs, associate a corresponding BITNode
	for (const Handle& leaf : get_leaves())
		insert_bitnode(leaf, BITNodeFitness());
}

AndBIT::HandleBITNodeMap::iterator
AndBIT::insert_bitnode(Handle leaf, const BITNodeFitness& fitness)
{
	if (not leaf)
		return leaf2bitnode.end();

	HandleBITNodeMap::iterator it = leaf2bitnode.find(leaf);
	if (it == leaf2bitnode.end())
		return leaf2bitnode.emplace(leaf, BITNode(leaf, fitness)).first;
	return it;
}

HandleSet AndBIT::get_leaves() const
{
	return get_leaves(fcs);
}

HandleSet AndBIT::get_leaves(const Handle& h) const
{
	Type t = h->get_type();
	if (t == BIND_LINK) {
		BindLinkPtr hsc = BindLinkCast(h);
		// Assume there is only one rewrite.
		Handle rewrite = hsc->get_implicand()[0];
		return get_leaves(rewrite);
	} else if (t == EXECUTION_OUTPUT_LINK) {
		// All arguments except the first one are potential target leaves
		Handle args = h->getOutgoingAtom(1);
		HandleSet leaves;
		if (args->get_type() == LIST_LINK) {
			OC_ASSERT(args->get_arity() > 0);
			for (Arity i = 1; i < args->get_arity(); i++) {
				HandleSet aleaves = get_leaves(args->getOutgoingAtom(i));
				leaves.insert(aleaves.begin(), aleaves.end());
			}
		}
		return leaves;
	} else if (t == SET_LINK) {
		// All atoms wrapped in a SetLink are potential target leaves
		HandleSet leaves;
		for (const Handle& el : h->getOutgoingSet()) {
			HandleSet el_leaves = get_leaves(el);
			leaves.insert(el_leaves.begin(), el_leaves.end());
		}
		return leaves;
	} else if (contains_atomtype(h, EXECUTION_OUTPUT_LINK)) {
		// If it contains an unquoted ExecutionOutputLink then it is
		// not a leaf (maybe it could but it would over complicate the
		// rest and bring no benefit since we can always expand a
		// parent and-BIT that has no such ExecutionOutputLink).
		return HandleSet{};
	}

	// Here it must be a leaf so return it
	return HandleSet{h};
}

Handle AndBIT::substitute_unified_variables(const Handle& leaf,
                                            const Unify::TypedSubstitution& ts) const
{
	BindLinkPtr fcs_bl(BindLinkCast(fcs));
	return Handle(Unify::substitute(fcs_bl, ts, queried_as));
}

Handle AndBIT::expand_fcs_pattern(const Handle& fcs_pattern,
                                  const Rule& rule) const
{
	Handle conclusion = rule.get_conclusion();
	HandleSeq prs_clauses = get_present_clauses(rule.get_implicant());
	HandleSeq virt_clauses = get_virtual_clauses(rule.get_implicant());
	HandleSeq prs_fcs_clauses = get_present_clauses(fcs_pattern);
	HandleSeq virt_fcs_clauses = get_virtual_clauses(fcs_pattern);

	// Remove any present fcs clauses that is equal to the conclusion
	auto eq_to_conclusion = [&](const Handle& h) {
		                        return content_eq(conclusion, h); };
	boost::remove_erase_if(prs_fcs_clauses, eq_to_conclusion);

	// Remove any virtual fcs clause that:
	// 1. is equal to the conclusion.
	// 2. is a precondition that uses that conclusion as argument.
	auto to_remove = [&](const Handle& h) {
		                 return eq_to_conclusion(h)
			                 or is_argument_of(h, conclusion); };
	boost::remove_erase_if(virt_fcs_clauses, to_remove);

	// Add present rule clauses
	prs_fcs_clauses.insert(prs_fcs_clauses.end(),
	                       prs_clauses.begin(), prs_clauses.end());

	// Add virtual rule clauses
	virt_fcs_clauses.insert(virt_fcs_clauses.end(),
	                        virt_clauses.begin(), virt_clauses.end());

	// Assemble the body
	return mk_pattern(prs_fcs_clauses, virt_fcs_clauses);
}

Handle AndBIT::expand_fcs_rewrite(const Handle& fcs_rewrite,
                                  const Rule& rule) const
{
	HandlePairSeq conclusions = rule.get_conclusions();
	OC_ASSERT(conclusions.size() == 1);
	Handle conclusion = conclusions[0].second;

	// Base cases

	// Replace the fcs rewrite atoms by the rule rewrite if equal to
	// the rule conclusion
	if (content_eq(fcs_rewrite, conclusion))
		return rule.get_implicand();

	// Recursive cases

	AtomSpace& as = *fcs->getAtomSpace();
	Type t = fcs_rewrite->get_type();

	if (t == EXECUTION_OUTPUT_LINK) {
		// If it is an ExecutionOutput then skip the first input
		// argument as it is a conclusion already.
		Handle gsn = fcs_rewrite->getOutgoingAtom(0);
		Handle arg = fcs_rewrite->getOutgoingAtom(1);
		if (arg->get_type() == LIST_LINK) {
			HandleSeq args = arg->getOutgoingSet();
			for (size_t i = 1; i < args.size(); i++)
				args[i] = expand_fcs_rewrite(args[i], rule);
			arg = as.add_link(LIST_LINK, std::move(args));
		}
		return as.add_link(EXECUTION_OUTPUT_LINK, {gsn, arg});
	} else if (t == SET_LINK) {
		// If a SetLink then treat its arguments as (unordered)
		// premises.
		HandleSeq args = fcs_rewrite->getOutgoingSet();
		for (size_t i = 0; i < args.size(); i++)
			args[i] = expand_fcs_rewrite(args[i], rule);
		return as.add_link(SET_LINK, std::move(args));
	} else
		// If none of the conditions apply just leave alone. Indeed,
		// assuming that the pattern matcher is executing the rewrite
		// term eagerly then it is garantied that all premises TVs
		// will be updated before running a rule, so we don't need to
		// substitute parts of a term containing the conclusion by the
		// application rule(premises).
		return fcs_rewrite;
}

bool AndBIT::is_argument_of(const Handle& eval, const Handle& atom) const
{
	if (eval->get_type() == EVALUATION_LINK) {
		Handle args = eval->getOutgoingAtom(1);
		if (content_eq(args, atom))
			return true;
		if (args->get_type() == LIST_LINK)
			for (Arity i = 0; i < args->get_arity(); i++)
				if (content_eq(args->getOutgoingAtom(i), atom))
					return true;
	}
	return false;
}

bool AndBIT::is_locally_quoted_eq(const Handle& lhs, const Handle& rhs) const
{
	if (content_eq(lhs, rhs))
		return true;
	Type lhs_t = lhs->get_type();
	Type rhs_t = rhs->get_type();
	if (lhs_t == LOCAL_QUOTE_LINK and rhs_t != LOCAL_QUOTE_LINK)
		return content_eq(lhs->getOutgoingAtom(0), rhs);
	if (lhs_t != LOCAL_QUOTE_LINK and rhs_t == LOCAL_QUOTE_LINK)
		return content_eq(lhs, rhs->getOutgoingAtom(0));
	return false;
}

Handle AndBIT::mk_pattern(HandleSeq prs_clauses, HandleSeq virt_clauses) const
{
	// Remove redundant clauses
	remove_redundant(prs_clauses);
	remove_redundant(virt_clauses);

	// Assemble the body
	AtomSpace& as = *fcs->getAtomSpace();
	if (not prs_clauses.empty())
		virt_clauses.push_back(as.add_link(PRESENT_LINK, std::move(prs_clauses)));
	return virt_clauses.empty() ? Handle::UNDEFINED
		: (virt_clauses.size() == 1 ? virt_clauses.front()
		   : as.add_link(AND_LINK, std::move(virt_clauses)));
}

void AndBIT::remove_redundant(HandleSeq& hs)
{
	boost::sort(hs);
	boost::erase(hs, boost::unique<boost::return_found_end>(hs));
}

HandleSeq AndBIT::get_present_clauses(const Handle& pattern)
{
	if (pattern->get_type() == AND_LINK)
		return get_present_clauses(pattern->getOutgoingSet());
	else if (pattern->get_type() == PRESENT_LINK)
		return pattern->getOutgoingSet();
	else
		return {};
}

HandleSeq AndBIT::get_present_clauses(const HandleSeq& clauses)
{
	HandleSeq prs_clauses;
	for (const Handle& clause : clauses) {
		if (clause->get_type() == PRESENT_LINK) {
			const HandleSeq& oset = clause->getOutgoingSet();
			prs_clauses.insert(prs_clauses.end(), oset.begin(), oset.end());
		}
	}
	return prs_clauses;
}

HandleSeq AndBIT::get_virtual_clauses(const Handle& pattern)
{
	if (pattern->get_type() == AND_LINK)
		return get_virtual_clauses(pattern->getOutgoingSet());
	else if (pattern->get_type() == PRESENT_LINK)
		return {};
	else
		return {pattern};
}

HandleSeq AndBIT::get_virtual_clauses(const HandleSeq& clauses)
{
	HandleSeq virt_clauses;
	for (const Handle& clause : clauses)
		if (clause->get_type() != PRESENT_LINK)
			virt_clauses.push_back(clause);
	return virt_clauses;
}

std::vector<std::string>
AndBIT::ascii_art_hmerge(const std::vector<std::string>& laa,
                         const std::vector<std::string>& raa,
                         unsigned dst)
{
	if (laa.empty())
		return raa;
	if (raa.empty())
		return laa;

	// Max line size of laa (in common with raa)
	int left_max = 0;

	// Min leading spaces of raa (in common with laa)
	int right_min = std::numeric_limits<int>::max();

	// Calculate the merging offset of origin of the right image
	// relative to the origin of the left one.
	size_t ls = laa.size();
	size_t rs = raa.size();
	for (size_t i = 0; i < std::min(ls, rs); ++i) {
		left_max = std::max(left_max, (int)laa[i].size());
		right_min = std::min(right_min, (int)leading_spaces(raa[i]));
	}

	// Only add dst if there is an actual border to not collide with.
	if (left_max) left_max += dst;

	// Where to paste the raa
	size_t offset = std::max(0, left_max - right_min);

	// Perform merging
	std::vector<std::string> result;
	for (size_t i = 0; i < std::max(ls, rs); ++i) {
		std::string line;
		if (i < ls)
			line = laa[i];
		if (i < rs) {
			// Fill with spaces
			while (line.size() < offset)
				line += " ";
			// Remove unused leading spaces
			line += i < ls ? raa[i].substr(line.size() - offset) : raa[i];
		}
		result.push_back(line);
	}
	return result;
}

std::string AndBIT::ascii_art_hmerge(const std::string& laa,
                                     const std::string& raa, unsigned dst)
{
	// Split laa into lines, from bottom to top
	std::vector<std::string> laa_lines = reverse_split(laa);
	// Split raa into lines, from bottom to top
	std::vector<std::string> raa_lines = reverse_split(raa);
	// Produce the merge and join it back into a string
	std::vector<std::string> res_lines =
		ascii_art_hmerge(laa_lines, raa_lines, dst);
	boost::reverse(res_lines);
	return boost::join(res_lines, "\n");
}

std::string AndBIT::ascii_art_hmerge(const std::vector<std::string>& aas,
                                     unsigned dst)
{
	std::string result;
	for (const std::string& aa : aas)
		result = ascii_art_hmerge(result, aa, dst);
	return result;
}

std::vector<std::string> AndBIT::reverse_split(const std::string& aa)
{
	std::vector<std::string> result;
	boost::split(result, aa, boost::is_any_of("\n"));
	boost::reverse(result);
	return result;
}

std::string AndBIT::bottom_line(const std::string& aa)
{
	std::vector<std::string> aa_lines = reverse_split(aa);
	return aa_lines.front();
}

unsigned AndBIT::leading_spaces(const std::string& line)
{
	std::string left_trimmed_line = boost::trim_left_copy(line);
	return line.size() - left_trimmed_line.size();
}

std::string AndBIT::remove_vowels(std::string str, size_t tg_size)
{
	std::string::size_type pos;

	// Remove lower-case vowels, starting at the end
	while (str.size() > tg_size) {
		if ((pos = str.find_last_of("aeiou")) == std::string::npos
			 or // Never remove the first character
			 pos == 0)
			break;
		str.replace (pos, 1, "");
	}
	return str;
}

std::string AndBIT::remove_consonants(std::string str, size_t tg_size)
{
	std::string::size_type pos;

	// Remove lower-case consonants, starting at the end
	while (str.size() > tg_size) {
		if ((pos = str.find_last_of("bcdfghjklmnpqrtvwxyz")) == std::string::npos
			 or // Never remove the first character
			 pos == 0)
			break;
		str.replace (pos, 1, "");
	}
	return str;
}

std::string AndBIT::abbreviate(std::string str, size_t tg_size)
{
	// We remove characters bit by bit in each word of the rule name,
	// starting from the end.
	std::vector<std::string> words;
	boost::split(words, str, boost::is_any_of("-"));

	for (int min_wrd_size = 3; 0 < min_wrd_size; min_wrd_size--) {
		for (std::string& word : boost::adaptors::reverse(words)) {
			int wrd_size = word.size();
			int rm_size = (int)str.size() - (int)tg_size;
			size_t wrd_tg_size = std::max(min_wrd_size, wrd_size - rm_size);
			word = remove_vowels(word, wrd_tg_size);
			word = remove_consonants(word, wrd_tg_size);
			str = boost::join(words, "-");
			if (str.size() == tg_size)
				return str;
		}
	}

	return str;
}

std::string AndBIT::line_separator(const std::string& up_aa,
                                   const std::string& low_aa,
                                   const Handle& gsn,
                                   bool unordered_premises)
{
	// Calculate the leading space and line separator sizes. We assume
	// that low_aa has no leading space.
	size_t lead_sp_size = 0;                // Leading space size
	size_t line_sep_size = low_aa.size();   // Line separator size
	if (not up_aa.empty()) {
		std::string up_bl = bottom_line(up_aa);
		size_t up_bls = up_bl.size();
		lead_sp_size = leading_spaces(up_bl);
		line_sep_size = std::max(line_sep_size, up_bls - lead_sp_size);
	}

	// Get formula string
	std::string lang, lib, fun;
	LibraryManager::parse_schema(gsn->get_name(), lang, lib, fun);
	std::string frml_str = fun;

	// Abbreviate formula string to fit inside the line separator
	size_t frml_str_max_size = line_sep_size;
	if (2 < frml_str_max_size)
		frml_str_max_size -= 2;
	std::string abbr_frml_str = abbreviate(frml_str, frml_str_max_size);
	size_t abbr_frml_str_size = abbr_frml_str.size();

	// Overlay the formula string on top of the line
	std::string line_str;
	size_t offset = (line_sep_size - abbr_frml_str_size) / 2;
	for (size_t i = 0; i < line_sep_size; ++i)
		if (i < offset or offset + abbr_frml_str_size <= i)
			line_str.push_back(unordered_premises ? '=' : '-');
		else
			line_str.push_back(abbr_frml_str[i - offset]);

	// Append leading space in front of the line
	return std::string(lead_sp_size, ' ') + line_str;
}

/////////
// BIT //
/////////

BIT::BIT() : _as(nullptr) {}

BIT::BIT(AtomSpace& as,
         const Handle& target,
         const Handle& vardecl,
         const BITNodeFitness& fitness)
	: bit_as(&as), // child atomspace of as
	  _as(&as), _init_target(target), _init_vardecl(vardecl),
	  _init_fitness(fitness) {}

BIT::~BIT() {}

bool BIT::empty() const
{
	return andbits.empty();
}

size_t BIT::size() const
{
	return andbits.size();
}

AndBIT* BIT::init()
{
	andbits.emplace_back(bit_as, _init_target,
	                     _init_vardecl, _init_fitness, _as);

	LAZY_URE_LOG_DEBUG << "Initialize BIT with:" << std::endl
	                   << andbits.begin()->to_string();

	return &*andbits.begin();
}

AndBIT* BIT::expand(AndBIT& andbit, BITNode& bitleaf,
                    const RuleTypedSubstitutionPair& rule, double prob)
{
	// Make sure that the rule is not already an or-child of bitleaf.
	if (contains(bitleaf, rule)) {
		ure_logger().debug() << "An equivalent rule has already expanded "
		                     << "that BIT-node, abort expansion";
		return nullptr;
	}

	// Insert the rule as or-branch of this bitleaf
	bitleaf.rules.insert(rule);

	// Expand the and-BIT and insert it in the BIT, if the expansion
	// was successful
	AndBIT new_andbit = andbit.expand(bitleaf.body, rule, prob);
	return (bool)new_andbit.fcs ? insert(new_andbit) : nullptr;
}

AndBIT* BIT::insert(AndBIT& andbit)
{
	// Check that it isn't already in the BIT
	if (boost::find(andbits, andbit) != andbits.end()) {
		LAZY_URE_LOG_DEBUG << "The following and-BIT is already in the BIT: "
		                   << andbit.fcs->id_to_string();
		return nullptr;
	}
	// Insert while keeping the order
	auto it = andbits.insert(boost::lower_bound(andbits, andbit), andbit);

	// Return andbit pointer
	return &*it;
}

void BIT::reset_exhausted_flags()
{
	for (AndBIT& andbit : andbits)
		andbit.reset_exhausted();
}

bool BIT::andbits_exhausted() const
{
	return boost::algorithm::all_of(andbits, [](const AndBIT& andbit) {
			return andbit.exhausted; });
}

bool BIT::contains(const BITNode& bitnode,
                   const RuleTypedSubstitutionPair& rule) const
{
	return bitnode.rules.find(rule.first) != bitnode.rules.end();
}

std::string oc_to_string(const BITNode& bitnode, const std::string& indent)
{
	return bitnode.to_string(indent);
}

std::string oc_to_string(const AndBIT& andbit, const std::string& indent)
{
	return andbit.to_string(indent);
}

// std::string oc_to_string(const BIT& bit)
// {
// 	return bit.to_string();
// }

} // ~namespace opencog
