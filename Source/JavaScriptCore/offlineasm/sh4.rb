# Copyright (C) 2013 Apple Inc. All rights reserved.
# Copyright (C) 2013 Cisco Systems, Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY CISCO SYSTEMS, INC. ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CISCO SYSTEMS, INC. OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

require 'risc'

class Node
    def sh4SingleHi
        doubleOperand = sh4Operand
        raise "Bogus register name #{doubleOperand}" unless doubleOperand =~ /^dr/
        "fr" + ($~.post_match.to_i).to_s
    end
    def sh4SingleLo
        doubleOperand = sh4Operand
        raise "Bogus register name #{doubleOperand}" unless doubleOperand =~ /^dr/
        "fr" + ($~.post_match.to_i + 1).to_s
    end
end

class SpecialRegister < NoChildren
    def sh4Operand
        @name
    end

    def dump
        @name
    end

    def register?
        true
    end
end

SH4_TMP_GPRS = [ SpecialRegister.new("r3"), SpecialRegister.new("r11"), SpecialRegister.new("r13") ]
SH4_TMP_FPRS = [ SpecialRegister.new("dr10") ]

class RegisterID
    def sh4Operand
        case name
        when "a0"
            "r4"
        when "a1"
            "r5"
        when "t0"
            "r0"
        when "t1"
            "r1"
        when "t2"
            "r2"
        when "t3"
            "r10"
        when "t4"
            "r6"
        when "cfr"
            "r14"
        when "sp"
            "r15"
        when "lr"
            "pr"
        else
            raise "Bad register #{name} for SH4 at #{codeOriginString}"
        end
    end
end

class FPRegisterID
    def sh4Operand
        case name
        when "ft0", "fr"
            "dr0"
        when "ft1"
            "dr2"
        when "ft2"
            "dr4"
        when "ft3"
            "dr6"
        when "ft4"
            "dr8"
        when "fa0"
            "dr12"
        else
            raise "Bad register #{name} for SH4 at #{codeOriginString}"
        end
    end
end

class Immediate
    def sh4Operand
        raise "Invalid immediate #{value} at #{codeOriginString}" if value < -128 or value > 127
        "##{value}"
    end
end

class Address
    def sh4Operand
        raise "Bad offset #{offset.value} at #{codeOriginString}" if offset.value < 0 or offset.value > 60
        if offset.value == 0
            "@#{base.sh4Operand}"
        else
            "@(#{offset.value}, #{base.sh4Operand})"
        end
    end

    def sh4OperandPostInc
        raise "Bad offset #{offset.value} for post inc at #{codeOriginString}" unless offset.value == 0
        "@#{base.sh4Operand}+"
    end

    def sh4OperandPreDec
        raise "Bad offset #{offset.value} for pre dec at #{codeOriginString}" unless offset.value == 0
        "@-#{base.sh4Operand}"
    end
end

class BaseIndex
    def sh4Operand
        raise "Unconverted base index at #{codeOriginString}"
    end
end

class AbsoluteAddress
    def sh4Operand
        raise "Unconverted absolute address at #{codeOriginString}"
    end
end


#
# Lowering of shift ops for SH4. For example:
#
# rshifti foo, bar
#
# becomes:
#
# negi foo, tmp
# shld tmp, bar
#

def sh4LowerShiftOps(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            case node.opcode
            when "ulshifti", "ulshiftp", "urshifti", "urshiftp", "lshifti", "lshiftp", "rshifti", "rshiftp"
                if node.opcode[0,1] == "u"
                    type = "l"
                    direction = node.opcode[1,1]
                else
                    type = "a"
                    direction = node.opcode[0,1]
                end
                if node.operands[0].is_a? Immediate
                    if node.operands[0].value == 0
                        # There is nothing to do here.
                    elsif node.operands[0].value == 1 or (type == "l" and [2, 8, 16].include? node.operands[0].value)
                        newList << Instruction.new(node.codeOrigin, "sh#{type}#{direction}x", node.operands)
                    else
                        tmp = Tmp.new(node.codeOrigin, :gpr)
                        if direction == "l"
                            newList << Instruction.new(node.codeOrigin, "move", [node.operands[0], tmp])
                        else
                            newList << Instruction.new(node.codeOrigin, "move", [Immediate.new(node.operands[0].codeOrigin, -1 * node.operands[0].value), tmp])
                        end
                        newList << Instruction.new(node.codeOrigin, "sh#{type}d", [tmp, node.operands[1]])
                    end
                else
                    if direction == "l"
                        newList << Instruction.new(node.codeOrigin, "sh#{type}d", node.operands)
                    else
                        tmp = Tmp.new(node.codeOrigin, :gpr)
                        newList << Instruction.new(node.codeOrigin, "negi", [node.operands[0], tmp])
                        newList << Instruction.new(node.codeOrigin, "sh#{type}d", [tmp, node.operands[1]])
                    end
                end
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end


#
# Lowering of simple branch ops for SH4. For example:
#
# baddis foo, bar, baz
#
# will become:
#
# addi foo, bar, tmp
# bs tmp, baz
#

def sh4LowerSimpleBranchOps(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            annotation = node.annotation
            case node.opcode
            when /^b(addi|subi|ori|addp)/
                op = $1
                bc = $~.post_match
                branch = "b" + bc

                case op
                when "addi", "addp"
                    op = "addi"
                when "subi"
                    op = "subi"
                when "ori"
                    op = "ori"
                end

                if bc == "s"
                    tmp = Tmp.new(node.codeOrigin, :gpr)
                    newList << Instruction.new(node.codeOrigin, op, [node.operands[0], node.operands[1], tmp])
                    newList << Instruction.new(node.codeOrigin, "bs", [tmp, node.operands[2]])
                else
                    newList << node
                end
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end


#
# Lowering of double accesses for SH4. For example:
#
# loadd [foo, bar, 8], baz
#
# becomes:
#
# leap [foo, bar, 8], tmp
# loaddReversedAndIncrementAddress [tmp], baz
#

def sh4LowerDoubleAccesses(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            case node.opcode
            when "loadd"
                tmp = Tmp.new(codeOrigin, :gpr)
                addr = Address.new(codeOrigin, tmp, Immediate.new(codeOrigin, 0))
                newList << Instruction.new(codeOrigin, "leap", [node.operands[0], tmp])
                newList << Instruction.new(node.codeOrigin, "loaddReversedAndIncrementAddress", [addr, node.operands[1]], node.annotation)
            when "stored"
                tmp = Tmp.new(codeOrigin, :gpr)
                addr = Address.new(codeOrigin, tmp, Immediate.new(codeOrigin, 0))
                newList << Instruction.new(codeOrigin, "leap", [node.operands[1].withOffset(8), tmp])
                newList << Instruction.new(node.codeOrigin, "storedReversedAndDecrementAddress", [node.operands[0], addr], node.annotation)
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end


#
# Lowering of double specials for SH4.
#

def sh4LowerDoubleSpecials(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            case node.opcode
            when "bdnequn", "bdgtequn", "bdltun", "bdltequn", "bdgtun"
                # Handle floating point unordered opcodes.
                tmp1 = Tmp.new(codeOrigin, :gpr)
                tmp2 = Tmp.new(codeOrigin, :gpr)
                newList << Instruction.new(codeOrigin, "bdnan", [node.operands[0], node.operands[2], tmp1, tmp2])
                newList << Instruction.new(codeOrigin, "bdnan", [node.operands[1], node.operands[2], tmp1, tmp2])
                newList << Instruction.new(codeOrigin, node.opcode[0..-3], node.operands)
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end


#
# Lowering of misplaced labels for SH4.
#

def sh4LowerMisplacedLabels(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            case node.opcode
            when "jmp"
                if node.operands[0].is_a? LabelReference
                    tmp = Tmp.new(codeOrigin, :gpr)
                    newList << Instruction.new(codeOrigin, "jmpf", [tmp, node.operands[0]])
                else
                    newList << node
                end
            when "call"
                if node.operands[0].is_a? LabelReference
                    tmp1 = Tmp.new(codeOrigin, :gpr)
                    tmp2 = Tmp.new(codeOrigin, :gpr)
                    newList << Instruction.new(codeOrigin, "callf", [tmp1, tmp2, node.operands[0]])
                else
                    newList << node
                end
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end


class Sequence
    def getModifiedListSH4
        result = @list

        # Verify that we will only see instructions and labels.
        result.each {
            | node |
            unless node.is_a? Instruction or
                    node.is_a? Label or
                    node.is_a? LocalLabel or
                    node.is_a? Skip
                raise "Unexpected #{node.inspect} at #{node.codeOrigin}"
            end
        }

        result = sh4LowerShiftOps(result)
        result = sh4LowerSimpleBranchOps(result)
        result = riscLowerMalformedAddresses(result) {
            | node, address |
            if address.is_a? Address
                case node.opcode
                when "btbz", "btbnz", "cbeq", "bbeq", "bbneq", "bbb", "loadb"
                    (0..15).include? address.offset.value and
                        ((node.operands[0].is_a? RegisterID and node.operands[0].sh4Operand == "r0") or
                         (node.operands[1].is_a? RegisterID and node.operands[1].sh4Operand == "r0"))
                when "loadh"
                    (0..30).include? address.offset.value and
                        ((node.operands[0].is_a? RegisterID and node.operands[0].sh4Operand == "r0") or
                         (node.operands[1].is_a? RegisterID and node.operands[1].sh4Operand == "r0"))
                else
                    (0..60).include? address.offset.value
                end
            else
                false
            end
        }
        result = sh4LowerDoubleAccesses(result)
        result = sh4LowerDoubleSpecials(result)
        result = riscLowerMisplacedImmediates(result, ["storeb", "storei", "storep", "muli", "mulp", "andi", "ori", "xori",
            "cbeq", "cieq", "cpeq", "cineq", "cpneq", "cib", "baddio", "bsubio", "bmulio", "baddis",
            "bbeq", "bbneq", "bbb", "bieq", "bpeq", "bineq", "bpneq", "bia", "bpa", "biaeq", "bpaeq", "bib", "bpb",
            "bigteq", "bpgteq", "bilt", "bplt", "bigt", "bpgt", "bilteq", "bplteq", "btiz", "btpz", "btinz", "btpnz", "btbz", "btbnz"])
        result = riscLowerMalformedImmediates(result, -128..127)
        result = sh4LowerMisplacedLabels(result)
        result = riscLowerMisplacedAddresses(result)

        result = assignRegistersToTemporaries(result, :gpr, SH4_TMP_GPRS)
        result = assignRegistersToTemporaries(result, :gpr, SH4_TMP_FPRS)

        return result
    end
end

def sh4Operands(operands)
    operands.map{|v| v.sh4Operand}.join(", ")
end

def emitSH4LoadConstant(constant, operand)
    if constant == 0x40000000
        # FirstConstantRegisterIndex const is often used (0x40000000).
        # It's more efficient to "build" the value with 3 opcodes without branch.
        $asm.puts "mov #64, #{operand.sh4Operand}"
        $asm.puts "shll16 #{operand.sh4Operand}"
        $asm.puts "shll8 #{operand.sh4Operand}"
    else
        constlabel = LocalLabel.unique("loadconstant")
        $asm.puts ".balign 4"
        $asm.puts "mov.l @(8, PC), #{operand.sh4Operand}"
        $asm.puts "bra #{LocalLabelReference.new(codeOrigin, constlabel).asmLabel}"
        $asm.puts "nop"
        $asm.puts "nop"
        $asm.puts ".long #{constant}"
        constlabel.lower("SH4")
    end
end

def emitSH4Branch(sh4opcode, operand)
    $asm.puts "#{sh4opcode} @#{operand.sh4Operand}"
    $asm.puts "nop"
end

def emitSH4ShiftImm(val, operand, direction)
    tmp = val
    while tmp > 0
        if tmp >= 16
            $asm.puts "shl#{direction}16 #{operand.sh4Operand}"
            tmp -= 16
        elsif tmp >= 8
            $asm.puts "shl#{direction}8 #{operand.sh4Operand}"
            tmp -= 8
        elsif tmp >= 2
            $asm.puts "shl#{direction}2 #{operand.sh4Operand}"
            tmp -= 2
        else
            $asm.puts "shl#{direction} #{operand.sh4Operand}"
            tmp -= 1
        end
    end
end

def emitSH4BranchIfT(label, neg)
    outlabel = LocalLabel.unique("branchIfT")
    sh4opcode = neg ? "bt" : "bf"
    $asm.puts "#{sh4opcode} #{LocalLabelReference.new(codeOrigin, outlabel).asmLabel}"
    if label.is_a? LocalLabelReference
        $asm.puts "bra #{label.asmLabel}"
        $asm.puts "nop"
    else
        $asm.puts ".balign 4"
        $asm.puts "mov.l @(8, PC), #{SH4_TMP_GPRS[0].sh4Operand}"
        $asm.puts "jmp @#{SH4_TMP_GPRS[0].sh4Operand}"
        $asm.puts "nop"
        $asm.puts "nop"
        $asm.puts ".long #{label.asmLabel}"
    end
    outlabel.lower("SH4")
end

def emitSH4IntCompare(cmpOpcode, operands)
    $asm.puts "cmp/#{cmpOpcode} #{sh4Operands([operands[1], operands[0]])}"
end

def emitSH4CondBranch(cmpOpcode, neg, operands)
    emitSH4IntCompare(cmpOpcode, operands)
    emitSH4BranchIfT(operands[2], neg)
end

def emitSH4CompareSet(cmpOpcode, neg, operands)
    emitSH4IntCompare(cmpOpcode, operands)
    $asm.puts "movt #{operands[2].sh4Operand}"
    if neg
        $asm.puts "dt #{operands[2].sh4Operand}"
    end
end

def emitSH4BranchIfNaN(operands)
    raise "Invalid operands number (#{operands.size})" unless operands.size == 4
    dblop = operands[0]
    labelop = operands[1]
    scrmask = operands[2]
    scrint = operands[3]

    # If we don't have "E = Emax + 1", it's not a NaN.
    notNaNlabel = LocalLabel.unique("notnan")
    $asm.puts "fcnvds #{dblop.sh4Operand}, fpul"
    $asm.puts "sts fpul, #{scrint.sh4Operand}"
    emitSH4LoadConstant(0x7f800000, scrmask)
    $asm.puts "and #{sh4Operands([scrmask, scrint])}"
    $asm.puts "cmp/eq #{sh4Operands([scrmask, scrint])}"
    $asm.puts "bf #{LocalLabelReference.new(codeOrigin, notNaNlabel).asmLabel}"

    # If we have "E = Emax + 1" and "f != 0", then it's a NaN.
    $asm.puts "sts fpul, #{scrint.sh4Operand}"
    emitSH4LoadConstant(0x003fffff, scrmask)
    $asm.puts "tst #{sh4Operands([scrmask, scrint])}"
    $asm.puts "bf #{labelop.asmLabel}"

    notNaNlabel.lower("SH4")
end

def emitSH4DoubleCondBranch(cmpOpcode, neg, operands)
    if cmpOpcode == "lt"
        if (!neg)
            outlabel = LocalLabel.unique("dcbout")
            $asm.puts "fcmp/gt #{sh4Operands([operands[1], operands[0]])}"
            $asm.puts "bt #{LocalLabelReference.new(codeOrigin, outlabel).asmLabel}"
            $asm.puts "fcmp/eq #{sh4Operands([operands[1], operands[0]])}"
            $asm.puts "bf #{operands[2].asmLabel}"
            outlabel.lower("SH4")
        else
            $asm.puts "fcmp/gt #{sh4Operands([operands[1], operands[0]])}"
            $asm.puts "bt #{operands[2].asmLabel}"
            $asm.puts "fcmp/eq #{sh4Operands([operands[1], operands[0]])}"
            $asm.puts "bt #{operands[2].asmLabel}"
        end
    else
        $asm.puts "fcmp/#{cmpOpcode} #{sh4Operands([operands[1], operands[0]])}"
        emitSH4BranchIfT(operands[2], neg)
    end
end

class Instruction
    def lowerSH4
        $asm.comment codeOriginString
        case opcode
        when "addi", "addp"
            if operands.size == 3
                if operands[0].sh4Operand == operands[2].sh4Operand
                    $asm.puts "add #{sh4Operands([operands[1], operands[2]])}"
                elsif operands[1].sh4Operand == operands[2].sh4Operand
                    $asm.puts "add #{sh4Operands([operands[0], operands[2]])}"
                else
                    $asm.puts "mov #{sh4Operands([operands[0], operands[2]])}"
                    $asm.puts "add #{sh4Operands([operands[1], operands[2]])}"
                end
            else
                $asm.puts "add #{sh4Operands(operands)}"
            end
        when "subi"
            raise "#{opcode} with #{operands.size} operands is not handled yet" unless operands.size == 2
            if operands[0].is_a? Immediate
                $asm.puts "add #{sh4Operands([Immediate.new(codeOrigin, -1 * operands[0].value), operands[1]])}"
            else
                $asm.puts "sub #{sh4Operands(operands)}"
            end
        when "muli", "mulp"
            $asm.puts "mul.l #{sh4Operands(operands[0..1])}"
            $asm.puts "sts macl, #{operands[-1].sh4Operand}"
        when "negi"
            if operands.size == 2
                $asm.puts "neg #{sh4Operands(operands)}"
            else
                $asm.puts "neg #{sh4Operands([operands[0], operands[0]])}"
            end
        when "andi", "ori", "xori"
            raise "#{opcode} with #{operands.size} operands is not handled yet" unless operands.size == 2
            sh4opcode = opcode[0..-2]
            $asm.puts "#{sh4opcode} #{sh4Operands(operands)}"
        when "shllx", "shlrx"
            raise "Unhandled parameters for opcode #{opcode}" unless operands[0].is_a? Immediate
            if operands[0].value == 1
                $asm.puts "shl#{opcode[3,1]} #{operands[1].sh4Operand}"
            else
                $asm.puts "shl#{opcode[3,1]}#{operands[0].value} #{operands[1].sh4Operand}"
            end
        when "shld", "shad"
            $asm.puts "#{opcode} #{sh4Operands(operands)}"
        when "loaddReversedAndIncrementAddress"
            # As we are little endian, we don't use "fmov @Rm, DRn" here.
            $asm.puts "fmov.s #{operands[0].sh4OperandPostInc}, #{operands[1].sh4SingleLo}"
            $asm.puts "fmov.s #{operands[0].sh4OperandPostInc}, #{operands[1].sh4SingleHi}"
        when "storedReversedAndDecrementAddress"
            # As we are little endian, we don't use "fmov DRm, @Rn" here.
            $asm.puts "fmov.s #{operands[0].sh4SingleHi}, #{operands[1].sh4OperandPreDec}"
            $asm.puts "fmov.s #{operands[0].sh4SingleLo}, #{operands[1].sh4OperandPreDec}"
        when "ci2d"
            $asm.puts "lds #{operands[0].sh4Operand}, fpul"
            $asm.puts "float fpul, #{operands[1].sh4Operand}"
        when "fii2d"
            $asm.puts "lds #{operands[0].sh4Operand}, fpul"
            $asm.puts "fsts fpul, #{operands[2].sh4SingleLo}"
            $asm.puts "lds #{operands[1].sh4Operand}, fpul"
            $asm.puts "fsts fpul, #{operands[2].sh4SingleHi}"
        when "fd2ii"
            $asm.puts "flds #{operands[0].sh4SingleLo}, fpul"
            $asm.puts "sts fpul, #{operands[1].sh4Operand}"
            $asm.puts "flds #{operands[0].sh4SingleHi}, fpul"
            $asm.puts "sts fpul, #{operands[2].sh4Operand}"
        when "addd", "subd", "muld", "divd"
            sh4opcode = opcode[0..-2]
            $asm.puts "f#{sh4opcode} #{sh4Operands(operands)}"
        when "bcd2i"
            $asm.puts "ftrc #{operands[0].sh4Operand}, fpul"
            $asm.puts "sts fpul, #{operands[1].sh4Operand}"
            $asm.puts "float fpul, #{SH4_TMP_FPRS[0].sh4Operand}"
            $asm.puts "fcmp/eq #{sh4Operands([operands[0], SH4_TMP_FPRS[0]])}"
            $asm.puts "bf #{operands[2].asmLabel}"
        when "bdnan"
            emitSH4BranchIfNaN(operands)
        when "bdneq"
            emitSH4DoubleCondBranch("eq", true, operands)
        when "bdgteq"
            emitSH4DoubleCondBranch("lt", true, operands)
        when "bdlt"
            emitSH4DoubleCondBranch("lt", false, operands)
        when "bdlteq"
            emitSH4DoubleCondBranch("gt", true, operands)
        when "bdgt"
            emitSH4DoubleCondBranch("gt", false, operands)
        when "baddio", "bsubio"
            raise "#{opcode} with #{operands.size} operands is not handled yet" unless operands.size == 3
            $asm.puts "#{opcode[1,3]}v #{sh4Operands([operands[0], operands[1]])}"
            $asm.puts "bt #{operands[2].asmLabel}"
        when "bmulio"
            $asm.puts "dmuls.l #{sh4Operands([operands[0], operands[1]])}"
            $asm.puts "sts mach, #{operands[-2].sh4Operand}"
            $asm.puts "tst #{sh4Operands([operands[-2], operands[-2]])}"
            $asm.puts "sts macl, #{operands[-2].sh4Operand}"
            $asm.puts "bf #{operands[-1].asmLabel}"
        when "btiz", "btpz", "btinz", "btpnz", "btbz", "btbnz"
            if operands.size == 3
                $asm.puts "tst #{sh4Operands([operands[0], operands[1]])}"
            else
                if operands[0].sh4Operand == "r0"
                    $asm.puts "cmp/eq #0, r0"
                else
                    $asm.puts "tst #{sh4Operands([operands[0], operands[0]])}"
                end
            end
            emitSH4BranchIfT(operands[-1], (opcode[-2,2] == "nz"))
        when "cbeq"
            emitSH4CompareSet("eq", false, operands)
        when "cieq", "cpeq"
            emitSH4CompareSet("eq", false, operands)
        when "cineq", "cpneq"
            emitSH4CompareSet("eq", true, operands)
        when "cib"
            emitSH4CompareSet("hs", true, operands)
        when "bbeq"
            emitSH4CondBranch("eq", false, operands)
        when "bbneq"
            emitSH4CondBranch("eq", true, operands)
        when "bbb"
            emitSH4CondBranch("hs", true, operands)
        when "bieq", "bpeq"
            emitSH4CondBranch("eq", false, operands)
        when "bineq", "bpneq"
            emitSH4CondBranch("eq", true, operands)
        when "bia", "bpa"
            emitSH4CondBranch("hi", false, operands)
        when "biaeq", "bpaeq"
            emitSH4CondBranch("hs", false, operands)
        when "bib", "bpb"
            emitSH4CondBranch("hs", true, operands)
        when "bigteq", "bpgteq"
            emitSH4CondBranch("ge", false, operands)
        when "bilt", "bplt"
            emitSH4CondBranch("ge", true, operands)
        when "bigt", "bpgt"
            emitSH4CondBranch("gt", false, operands)
        when "bilteq", "bplteq"
            emitSH4CondBranch("gt", true, operands)
        when "bs"
            $asm.puts "cmp/pz #{operands[0].sh4Operand}"
            $asm.puts "bf #{operands[1].asmLabel}"
        when "call"
            if operands[0].is_a? LocalLabelReference
                $asm.puts "bsr #{operands[0].asmLabel}"
                $asm.puts "nop"
            elsif operands[0].is_a? RegisterID or operands[0].is_a? SpecialRegister
                emitSH4Branch("jsr", operands[0])
            else
                raise "Unhandled parameters for opcode #{opcode} at #{codeOriginString}"
            end
        when "callf"
            $asm.puts ".balign 4"
            $asm.puts "mov r0, #{operands[0].sh4Operand}"
            $asm.puts "mova @(14, PC), r0"
            $asm.puts "lds r0, pr"
            $asm.puts "mov.l @(6, PC), #{operands[1].sh4Operand}"
            $asm.puts "jmp @#{operands[1].sh4Operand}"
            $asm.puts "mov #{operands[0].sh4Operand}, r0"
            $asm.puts ".long #{operands[2].asmLabel}"
        when "jmp"
            if operands[0].is_a? LocalLabelReference
                $asm.puts "bra #{operands[0].asmLabel}"
                $asm.puts "nop"
            elsif operands[0].is_a? RegisterID or operands[0].is_a? SpecialRegister
                emitSH4Branch("jmp", operands[0])
            else
                raise "Unhandled parameters for opcode #{opcode} at #{codeOriginString}"
            end
        when "jmpf"
            $asm.puts ".balign 4"
            $asm.puts "mov.l @(8, PC), #{operands[0].sh4Operand}"
            $asm.puts "jmp @#{operands[0].sh4Operand}"
            $asm.puts "nop"
            $asm.puts "nop"
            $asm.puts ".long #{operands[1].asmLabel}"
        when "ret"
            $asm.puts "rts"
            $asm.puts "nop"
        when "loadb"
            $asm.puts "mov.b #{sh4Operands(operands)}"
            $asm.puts "extu.b #{sh4Operands([operands[1], operands[1]])}"
        when "loadh"
            $asm.puts "mov.w #{sh4Operands(operands)}"
            $asm.puts "extu.w #{sh4Operands([operands[1], operands[1]])}"
        when "loadi", "loadis", "loadp", "storei", "storep"
            $asm.puts "mov.l #{sh4Operands(operands)}"
        when "move"
            if operands[0].is_a? Immediate and (operands[0].value < -128 or operands[0].value > 127)
                emitSH4LoadConstant(operands[0].value, operands[1])
            elsif operands[0].is_a? LabelReference
                emitSH4LoadConstant(operands[0].asmLabel, operands[1])
            else
                $asm.puts "mov #{sh4Operands(operands)}"
            end
        when "leap"
            if operands[0].is_a? BaseIndex
                biop = operands[0]
                if biop.scale > 0
                    $asm.puts "mov #{sh4Operands([biop.index, operands[1]])}"
                    if biop.scaleShift > 0
                        emitSH4ShiftImm(biop.scaleShift, operands[1], "l")
                    end
                    $asm.puts "add #{sh4Operands([biop.base, operands[1]])}"
                else
                    $asm.puts "mov #{sh4Operands([biop.base, operands[1]])}"
                end
                if biop.offset.value != 0
                    $asm.puts "add #{sh4Operands([biop.offset, operands[1]])}"
                end
            elsif operands[0].is_a? Address
                if operands[0].base != operands[1]
                    $asm.puts "mov #{sh4Operands([operands[0].base, operands[1]])}"
                end
                if operands[0].offset.value != 0
                    $asm.puts "add #{sh4Operands([operands[0].offset, operands[1]])}"
                end
            else
                raise "Unhandled parameters for opcode #{opcode} at #{codeOriginString}"
            end
        when "ldspr"
            $asm.puts "lds #{sh4Operands(operands)}, pr"
        when "stspr"
            $asm.puts "sts pr, #{sh4Operands(operands)}"
        when "break"
            # This special opcode always generates an illegal instruction exception.
            $asm.puts ".word 0xfffd"
        else
            raise "Unhandled opcode #{opcode} at #{codeOriginString}"
        end
    end
end

