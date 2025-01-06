#ifndef IMATERIAL_H
#define IMATERIAL_H

// See https://www.gdcvault.com/play/1024418/Efficient-Texture-Streaming-in-Titanfall
#define MATERIAL_HISTOGRAM_BIN_COUNT 16

abstract_class IMaterial
{
public:
	virtual const char*		GetName() const = 0;
	virtual uint8_t			GetMaterialType() const = 0;

	virtual const char*		GetNullString() const = 0;
	virtual int64_t			ReturnZero() const = 0;

	virtual void*			sub_1403B41A0(void* unk) = 0; // IDK

	virtual int				GetMappingWidth() const = 0;
	virtual int				GetMappingHeight() const = 0;

private:
	//TODO! <-- most of these are bitwise and operators testing flags of the member CMaterialGlue::unkFlags.
	// Don't call these without reversing/renaming first, as the const qualifier might have to be removed.
	virtual void stub_0() const = 0;
	virtual void stub_1() const = 0;
	virtual void stub_2() const = 0;
	virtual void stub_3() const = 0;
	virtual void stub_4() const = 0;
	virtual void stub_5() const = 0;
	virtual void stub_6() const = 0;
	virtual void stub_7() const = 0;
	virtual void stub_8() const = 0;
	virtual void stub_9() const = 0;
	virtual void stub_10() const = 0;
	virtual void stub_11() const = 0;
	virtual void stub_12() const = 0;
	virtual void stub_13() const = 0;
	virtual void stub_14() const = 0;
	virtual void stub_15() const = 0;
	virtual void stub_16() const = 0;
	virtual void stub_17() const = 0;
	virtual void stub_18() const = 0;
	virtual void stub_19() const = 0;
	virtual void stub_20() const = 0;
	virtual void stub_21() const = 0;
	virtual void stub_22() const = 0;
	virtual void stub_23() const = 0;
	virtual void stub_24() const = 0;
	virtual void stub_25() const = 0;
	virtual void stub_26() const = 0;
	virtual void stub_27() const = 0;
	virtual void stub_28() const = 0;
	virtual void stub_29() const = 0;
	virtual void stub_30() const = 0;
	virtual void stub_31() const = 0;
	virtual void stub_32() const = 0;
	virtual void stub_33() const = 0;
	virtual void stub_34() const = 0;
	virtual void stub_35() const = 0;
	virtual void stub_36() const = 0;
	virtual void stub_37() const = 0;
	virtual void stub_38() const = 0;
	virtual void stub_39() const = 0;
	virtual void stub_40() const = 0;
	virtual void stub_41() const = 0;
	virtual void stub_42() const = 0;
	virtual void stub_43() const = 0;
	virtual void stub_44() const = 0;
	virtual void stub_45() const = 0;
	virtual void stub_46() const = 0;
	virtual void stub_47() const = 0;
	virtual void stub_48() const = 0;
	virtual void stub_49() const = 0;
	virtual void stub_50() const = 0;
	virtual void stub_51() const = 0;
	virtual void stub_52() const = 0;
	virtual void stub_53() const = 0;
	virtual void stub_54() const = 0;
	virtual void stub_55() const = 0;
	virtual void stub_56() const = 0;
	virtual void stub_57() const = 0;
	virtual void stub_58() const = 0;
	virtual void stub_59() const = 0;
	virtual void stub_60() const = 0;
	virtual void stub_61() const = 0;
	virtual void stub_62() const = 0;
	virtual void stub_63() const = 0;
	virtual void stub_64() const = 0;
	virtual void stub_65() const = 0;
	virtual void stub_66() const = 0;
	virtual void stub_67() const = 0;
	virtual void stub_68() const = 0;
	virtual void stub_69() const = 0;
	virtual void stub_70() const = 0;
	virtual void stub_71() const = 0;
	virtual void stub_72() const = 0;
	virtual void stub_73() const = 0;
	virtual void stub_74() const = 0;
	virtual void stub_75() const = 0;
	virtual void stub_76() const = 0;
	virtual void stub_77() const = 0;
	virtual void stub_78() const = 0;
	// STUB_138 should be GetShaderGlue.
};

#endif // IMATERIAL_H
